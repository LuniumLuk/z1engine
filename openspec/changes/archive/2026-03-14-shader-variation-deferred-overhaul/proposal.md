## Why

The deferred renderer's G-buffer pass currently renders nothing visible because it does not use the material shaders that assets are actually bound to. Here is the problem chain:

1. **Assets reference material shaders.** Every mesh primitive holds a `MaterialInstance` whose parent `Material` points to one of the four "surface shaders": `pbr.glsl`, `pbr_sg.glsl`, `phone.glsl`, or `unlit.glsl`. These shaders output a single `frag_color` (RGBA) -- they compute final lit color in the fragment stage (forward shading).

2. **The deferred renderer ignores those shaders.** In `renderer_deferred.cpp:192`, `mesh->draw(per_frame, m_default_material, filter)` dispatches draw calls through `StaticMesh::draw_primitive()` (`mesh.cpp:144`), which looks up each primitive's `MaterialInstance` and calls `mi->bind()`. This binds the material's own pipeline+shader (e.g. `pbr.glsl`), not the G-buffer shader. The G-buffer pipeline (`m_pipeline_gbuffer` loaded from `gbuffer.glsl`) is never bound during geometry rendering.

3. **Even if the G-buffer shader were forced**, it would only work for metallic-roughness PBR. There is no mechanism to select between PBR, specular-glossiness PBR, Phong, or unlit output paths at the shader level based on the material type. Each material blindly uses its own shader, and there is no concept of shader variants that could redirect the same material to different output paths (forward lit vs. G-buffer write) depending on the active render pass.

The root cause is that the engine has no **shader variation system** -- no way for a single material to produce different shader programs for different rendering contexts (forward pass, G-buffer pass, shadow pass, velocity pass). Without this, the deferred pipeline cannot reuse existing materials.

## What Changes

### 1. Shader Variation System

Introduce a bitmap-based shader variation mechanism that allows a single `.glsl` shader file to produce multiple compiled programs by injecting `#define` macros before compilation.

**Variation key**: A `uint32_t` bitmask where each bit (or bit range) maps to a specific macro injection. The engine defines a fixed set of variation bits:

| Bit(s) | Macro | Purpose |
|--------|-------|---------|
| 0 | `VARIANT_GBUFFER` | Output to G-buffer MRT instead of computing lighting |
| 1 | `VARIANT_SHADOW` | Shadow depth-only pass (replaces current `SHADOW` define) |
| 2 | `VARIANT_VELOCITY` | Velocity pass (replaces current `VELOCITY` define) |

The mapping is defined in a new header `engine/runtime/source/render/shader_variant.h`:

```cpp
namespace z1 {
    namespace ShaderVariant {
        constexpr uint32_t None     = 0;
        constexpr uint32_t GBuffer  = 1 << 0;
        constexpr uint32_t Shadow   = 1 << 1;
        constexpr uint32_t Velocity = 1 << 2;
    }
}
```

**Shader compilation changes** (`opengl_shader.cpp`): The `OpenGLShader` constructor that takes a `Filepath` currently compiles a single program. This is refactored so that:

- `Shader` gains a new factory: `static std::shared_ptr<Shader> create(Filepath const& path, uint32_t variant_key)`.
- Before prepending the uniform block to each stage source, the loader injects `#define` lines corresponding to set bits in `variant_key` (e.g. if bit 0 is set, prepend `#define VARIANT_GBUFFER 1\n`).
- The existing `Shader::create(path)` call (variant 0) continues to work unchanged for non-material shaders (post-process, TAA, bloom, etc.).

**Material-level caching**: `Material` already pools `Pipeline` objects by flag bitmask (`m_pipeline_pool`). This is extended to a two-level key: `(material_flags, variant_key)`. When the renderer needs a G-buffer variant, it requests `material->get_pipeline(flags, ShaderVariant::GBuffer)`. On first request, the material compiles the shader with that variant key and caches the resulting pipeline. The variant shader is compiled from the same `.glsl` source file as the base shader, just with extra defines.

**Shader storage**: A `ShaderLibrary` or cache (can be a simple `std::unordered_map<uint64_t, std::shared_ptr<Shader>>` keyed on `hash(path, variant_key)`) is added to avoid recompiling the same variant across materials that share the same base shader. This can live on `AssetManager` or as a standalone utility.

### 2. Modify Surface Shaders to Support G-Buffer Output

The four surface shaders are modified to branch at the **fragment stage** based on the `VARIANT_GBUFFER` define:

**`pbr.glsl` and `pbr_sg.glsl`** -- Fragment stage:
```glsl
@stage: frag {
    #include <include/frag_attrs.glsl>
    #include <include/lighting.glsl>

#ifdef VARIANT_GBUFFER
    #include <include/gbuffer_out.glsl>
    // Calls gbuffer_write() which writes to MRT
#else
    #include <include/pbr_frag.glsl>
    // Existing forward-lit output
#endif
}
```

**`phone.glsl`** -- Fragment stage:
```glsl
@stage: frag {
    #include <include/frag_attrs.glsl>
    #include <include/lighting.glsl>

#ifdef VARIANT_GBUFFER
    #include <include/gbuffer_out.glsl>
#else
    // Existing Phong forward-lit main()
#endif
}
```

**`unlit.glsl`** -- Fragment stage: Unlit materials write albedo to the G-buffer with metallic=0, roughness=1 and an emissive flag, or simply output color in forward mode.

A new shared include `include/gbuffer_out.glsl` is created that contains:
- MRT layout declarations (`layout(location = 1) out vec4 gbuffer_normal;` etc.)
- A `main()` function (or inline block) that extracts material properties (base color, normal, metallic, roughness) from the material's uniforms/textures and writes them to the G-buffer render targets.
- This replaces the current standalone `gbuffer.glsl` shader, which was a hardcoded PBR-only G-buffer writer. The logic from `gbuffer.glsl`'s fragment stage moves into `include/gbuffer_out.glsl` (for PBR metallic-roughness) and `include/gbuffer_out_sg.glsl` (for specular-glossiness, converting to metallic-roughness for the G-buffer).

The vertex stage is unchanged -- all surface shaders already use `#include <include/vert.glsl>` which handles skinning, world-space transform, and optional velocity/shadow outputs.

### 3. Update the Deferred Renderer to Use Material Shaders with Variants

The current G-buffer pass in `renderer_deferred.cpp` binds `m_pipeline_gbuffer` (a single hardcoded pipeline) and then calls `mesh->draw()` which internally binds each material's own pipeline, overriding the G-buffer pipeline. This is the core bug.

The fix: Instead of using a separate G-buffer pipeline, the renderer tells the material system to use the `GBuffer` variant. This is done by adding the variant key to `PerFrameConst`:

```cpp
struct PerFrameConst {
    glm::mat4 model;
    uint32_t global_binding;
    uint32_t lights_binding;
    uint32_t shadow_map_binding;
    uint32_t variant_key = 0;  // NEW: shader variant to use
};
```

`MaterialInstance::bind()` is modified to pass `per_frame.variant_key` to `Material::get_pipeline(flags, variant_key)`, which selects the correct compiled shader variant.

The G-buffer pass becomes:
```cpp
per_frame.variant_key = ShaderVariant::GBuffer;
// Now mesh->draw(per_frame, ...) will bind each material's G-buffer variant shader
```

The forward transparency pass and forward renderer set `variant_key = ShaderVariant::None` (default), so they continue using the standard forward-lit shaders.

`m_pipeline_gbuffer` is removed from `RendererDeferred` -- it is no longer needed since each material compiles its own G-buffer variant.

### 4. Unify Shadow and Velocity Passes Under the Variation System

The current shadow pass (`shadow.glsl`) and velocity pass (`velocity.glsl`) are standalone shaders with their own `#define SHADOW` and `#define VELOCITY` conventions in `include/vert.glsl`. These are migrated to the variation system:

- `shadow.glsl` is retired. Shadow rendering uses `per_frame.variant_key = ShaderVariant::Shadow`, which injects `#define VARIANT_SHADOW`. The surface shader vertex stage already handles `#ifdef SHADOW` for the sun projection; this is renamed to `#ifdef VARIANT_SHADOW`.
- `velocity.glsl` is retired. Velocity rendering uses `per_frame.variant_key = ShaderVariant::Velocity`, injecting `#define VARIANT_VELOCITY`. The vertex stage already handles `#ifdef VELOCITY`; renamed to `#ifdef VARIANT_VELOCITY`.

The fragment stages for shadow and velocity variants are added to the surface shaders via new shared includes (`include/shadow_out.glsl`, `include/velocity_out.glsl`), guarded by the respective variant defines.

This means the shadow and velocity passes now correctly use each material's own alpha mask textures and skinning without needing special-case shader binding in `RenderShared`.

### 5. Retire Standalone `gbuffer.glsl`

The standalone `gbuffer.glsl` shader is no longer needed since G-buffer output is now a variant of each surface shader. It can be removed or kept as a reference. The deferred lighting shader (`deferred_lighting.glsl`) remains unchanged -- it is a fullscreen quad shader that reads the G-buffer, not a material shader.

### 6. Shader Variation Validation (shader_validator)

All shader variations must compile and link successfully. The `shader_validator` tool is modified to automatically discover and test every variant combination declared by a shader file.

**Variant declaration in `.glsl` files**: Each shader that supports variants declares them with a new `@variants:` section at the top level, alongside `@uniforms:` and `@stage:`:

```glsl
@variants: {
    VARIANT_GBUFFER
    VARIANT_SHADOW
    VARIANT_VELOCITY
}

@uniforms: {
    // ...
}

@stage: vert {
    // ...
}

@stage: frag {
    // ...
}
```

The `@variants:` section lists the variant define names the shader supports, one per line. Not all shaders declare all variants. For example, `deferred_lighting.glsl` and post-process shaders declare no `@variants:` section (they have no variants). Surface shaders like `pbr.glsl` declare all three. A shader that only supports shadow casting but not G-buffer output would declare only `VARIANT_SHADOW`.

**shader_validator changes** (`engine/tool/shader_validator/main.cpp`):

1. **Parse `@variants:`**: After reading the shader source, the validator searches for an `@variants:` section using the same bracket-parsing logic as `@uniforms:` and `@stage:`. It extracts the list of variant define names.

2. **Enumerate all combinations**: For N declared variants, the validator generates all 2^N combinations (the power set). For example, a shader declaring `VARIANT_GBUFFER`, `VARIANT_SHADOW`, `VARIANT_VELOCITY` produces 8 combinations:
   - (none)
   - `VARIANT_GBUFFER`
   - `VARIANT_SHADOW`
   - `VARIANT_GBUFFER` + `VARIANT_SHADOW`
   - `VARIANT_VELOCITY`
   - `VARIANT_GBUFFER` + `VARIANT_VELOCITY`
   - `VARIANT_SHADOW` + `VARIANT_VELOCITY`
   - `VARIANT_GBUFFER` + `VARIANT_SHADOW` + `VARIANT_VELOCITY`

3. **Compile and link each combination**: For each combination, the validator prepends the corresponding `#define VARIANT_X 1\n` lines to the stage sources (after uniforms, before the stage code — the same injection point the engine's runtime compiler uses) and runs the full compile+link cycle.

4. **All must pass**: If any single variant combination fails to compile or link, the shader is reported as invalid. The validator prints which combination failed and the relevant compiler/linker error.

5. **Output format**: For each shader file, the validator prints:
   ```
   INFO:  Validating: engine/content/shader/pbr.glsl
   INFO:  Found variants: VARIANT_GBUFFER, VARIANT_SHADOW, VARIANT_VELOCITY (8 combinations)
   INFO:  [1/8] variant: (base) -- Compiling stage: vert ... frag ... Linking ... OK
   INFO:  [2/8] variant: VARIANT_GBUFFER -- Compiling stage: vert ... frag ... Linking ... OK
   ...
   INFO:  [8/8] variant: VARIANT_GBUFFER | VARIANT_SHADOW | VARIANT_VELOCITY -- ... OK
   INFO:  SUCCESS: All 8 variant combinations validated.
   ```

   Shaders with no `@variants:` section are validated exactly as before (single base variant).

**Restriction**: This is a hard gate. No shader may be checked in with a `@variants:` section that contains variant combinations that fail to compile or link. The shader_validator exit code reflects this — any failure returns non-zero.

## Capabilities

### New Capabilities
- `shader-variation`: Compile-time shader variant system using a bitmask of `#define` injections. Allows materials to produce different GPU programs for different rendering contexts (forward, G-buffer, shadow, velocity) from a single shader source.

### Modified Capabilities
- `deferred-rendering`: G-buffer pass now correctly renders all material types (PBR, PBR-SG, Phong, Unlit) by using shader variants instead of a single hardcoded G-buffer shader. Transparency forward pass is unchanged.

## File Organization (After Change)

```
engine/runtime/source/render/
  shader_variant.h             -- ShaderVariant bitmask constants
  shader.h/.cpp                -- Shader::create(path, variant_key) overload
  rhi/
    opengl_shader.h/.cpp       -- Variant define injection in constructor
  pipeline.h/.cpp              -- unchanged
  renderer/
    renderer_deferred.h/.cpp   -- G-buffer pass uses variant_key, m_pipeline_gbuffer removed
    renderer_forward.h/.cpp    -- unchanged (variant_key = 0 by default)
  render_shared.h/.cpp         -- Shadow/velocity passes use variant_key

engine/runtime/source/asset/
  material.h/.cpp              -- Pipeline pool keyed by (flags, variant_key), variant shader caching

engine/tool/shader_validator/
  main.cpp                     -- Parse @variants: section, enumerate 2^N combinations, validate all

engine/content/shader/
  pbr.glsl                     -- @variants: section added; fragment #ifdef VARIANT_GBUFFER branch added
  pbr_sg.glsl                  -- @variants: section added; fragment #ifdef VARIANT_GBUFFER branch added
  phone.glsl                   -- @variants: section added; fragment #ifdef VARIANT_GBUFFER branch added
  unlit.glsl                   -- @variants: section added; fragment #ifdef VARIANT_GBUFFER branch added
  gbuffer.glsl                 -- RETIRED (logic moved to include/gbuffer_out.glsl)
  shadow.glsl                  -- RETIRED (logic moved to surface shader variants)
  velocity.glsl                -- RETIRED (logic moved to surface shader variants)
  deferred_lighting.glsl       -- unchanged
  include/
    vert.glsl                 -- SHADOW -> VARIANT_SHADOW, VELOCITY -> VARIANT_VELOCITY
    gbuffer_out.glsl          -- NEW: G-buffer MRT output for PBR metallic-roughness
    gbuffer_out_sg.glsl       -- NEW: G-buffer MRT output for specular-glossiness (converts to MR)
    gbuffer_out_phone.glsl    -- NEW: G-buffer MRT output for Phong (approximates MR)
    gbuffer_out_unlit.glsl    -- NEW: G-buffer MRT output for unlit (emissive path)
    shadow_out.glsl           -- NEW: Shadow fragment (alpha mask discard)
    velocity_out.glsl         -- NEW: Velocity fragment (motion vector output)
    uniforms.glsl             -- unchanged
    lighting.glsl             -- unchanged
    pbr_frag.glsl             -- unchanged
    frag_attrs.glsl           -- unchanged
    quad.glsl                 -- unchanged
```

## Impact

- **`shader.h/.cpp` + `opengl_shader.h/.cpp`**: New variant-aware constructor/factory. Existing single-argument `create(path)` unchanged (defaults to variant 0).
- **`material.h/.cpp`**: Pipeline pool key changes from `uint32_t` to `uint64_t` (packing flags + variant_key). `MaterialInstance::bind()` reads variant from `PerFrameConst`.
- **`renderer_deferred.h/.cpp`**: Simplified -- no more `m_pipeline_gbuffer`. G-buffer pass sets `variant_key` on `PerFrameConst`.
- **`render_shared.h/.cpp`**: Shadow and velocity passes set `variant_key` instead of binding standalone shaders.
- **`mesh.cpp`**: `draw_primitive()` unchanged in interface but now respects the variant key passed through `PerFrameConst`.
- **Surface shaders** (`pbr.glsl`, `pbr_sg.glsl`, `phone.glsl`, `unlit.glsl`): New `@variants:` declaration section. Fragment stage gains `#ifdef VARIANT_GBUFFER` / `#ifdef VARIANT_SHADOW` / `#ifdef VARIANT_VELOCITY` branches. Forward-lit path unchanged.
- **`include/vert.glsl`**: `SHADOW` renamed to `VARIANT_SHADOW`, `VELOCITY` renamed to `VARIANT_VELOCITY`.
- **`shader_validator` (`engine/tool/shader_validator/main.cpp`)**: Parses new `@variants:` section, enumerates all 2^N define combinations, compiles and links each. All combinations must pass. Shaders without `@variants:` validated as before (base variant only).
- **Memory**: Each unique (shader_source, variant_key) pair compiles a separate GL program. With 4 surface shaders and 4 variants (none, gbuffer, shadow, velocity), this is up to 16 programs. Negligible memory overhead.

## Non-goals

- Runtime shader hot-reload with variant awareness (future enhancement).
- Per-material custom variant bits (the current design uses engine-defined bits only).
- Compute shader variants or geometry shader variants (not needed for the deferred pipeline).
- Changing the `.glsl` file format -- the `@uniforms`/`@stage`/`@reflections` structure is preserved.
- Tile-based or clustered deferred rendering.
- Additional deferred-enabled effects (SSAO, SSR) -- these build on the G-buffer but are separate work.

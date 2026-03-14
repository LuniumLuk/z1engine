## Why

`RendererForward` (`renderer_forward.cpp`, 965 lines) contains all rendering logic in a single struct: frustum culling, draw list building, light collection, CSM shadow calculation, the forward-lit main pass, velocity pass, TAA, bloom, and post-processing. Adding a deferred renderer would require duplicating most of this code. The current `MAX_LIGHTS = 16` cap with per-fragment lighting in the forward main pass also limits scene complexity. A deferred pipeline decouples light count from geometry cost, but the real prerequisite is extracting the shared infrastructure so both renderers can coexist without duplication.

## What Changes

### 1. Extract Shared Rendering Utilities from `RendererForward`

The following logic in `renderer_forward.cpp` is not forward-specific and should be extracted into shared modules under `engine/runtime/source/render/`:

- **`render_utils.h/.cpp`** -- Frustum culling (`Plane`, `Frustum`, `create_frustum`, `is_aabb_in_frustum`, `is_mesh_visible`, `get_skeletal_bounds`), Halton sequence generation, and draw list building. The `VisibleStaticMesh`, `VisibleSkeletalMesh`, and `VisibleDrawList` structs move here from `renderer_forward.h`.

- **`render_shared.h/.cpp`** -- Shared pass infrastructure that both renderers reuse:
  - Light collection and UBO upload (`LightData`, `LightsBlock`, `MAX_LIGHTS`, the current `update_lights` logic).
  - CSM cascade calculation (the current `calculate_csm_splits` logic).
  - Shadow pass setup (`add_shadow_pass` -- shadow pipeline, shadow framebuffer, shadow image).
  - Velocity pass (`add_velocity_pass` -- motion vectors for TAA).
  - TAA pass (`add_taa_pass` -- history buffer management, jitter application).
  - Bloom pass (`add_bloom_pass` -- downsample/upsample mip chain).
  - Post-processing pass (`add_postprocess_pass` -- tonemap, bloom composite, gamma).
  - Fullscreen quad VAO creation and management.

### 2. Slim Down `RendererForward`

After extraction, `RendererForward` retains only:
- Construction of forward-specific pipelines (material-driven main pass, skybox).
- The `draw()` orchestration method.
- `add_main_pass()` -- the forward-lit geometry pass (opaque/mask then blend, per-material shader dispatch, skybox rendering).

All shared pipeline objects (shadow, velocity, TAA, bloom, post-process), buffers (lights UBO, shadow image), and resources (quad VAO, history buffers, bloom textures) come from the shared module.

### 3. Add New `ImageFormat` Entries

The current `ImageFormat` enum (`image.h`) only has `RGBA8`, `RGBA32F`, `Depth`, `DepthStencil`. The G-buffer needs efficient storage:
- Add `RGB16F` -- for world-space position (3-channel, half-float).
- Add `RG16F` -- for metallic/roughness or encoded normals (2-channel, half-float).

Corresponding OpenGL mappings must be added in `rhi/opengl_image.cpp` and `rhi/opengl_framebuffer.cpp`.

### 4. Create `RendererDeferred`

New files: `engine/runtime/source/render/renderer/renderer_deferred.h/.cpp`.

The deferred renderer reuses all shared infrastructure and adds:
- **G-buffer generation pass**: Renders scene geometry to a multi-render-target framebuffer outputting position (`RGB16F`), normal (`RGB16F`), albedo (`RGBA8`), and metallic-roughness (`RG16F`). Uses new G-buffer shaders that write material properties instead of computing lighting.
- **Deferred lighting pass**: A fullscreen quad pass that reads the G-buffer textures, shadow map, and lights UBO to compute PBR/Phong lighting. Directional sun + up to `MAX_LIGHTS` dynamic lights in a single fragment shader without per-object overhead.
- **Forward transparency pass**: Blended objects (`AlphaMode::Blend`) cannot be deferred. These are rendered in a follow-up forward pass after the deferred lighting, composited onto the lit result.
- The remaining pipeline (velocity, TAA, bloom, post-process) is identical and comes from the shared module.

### 5. New Shaders

Under `engine/content/shader/`:
- **`gbuffer.glsl`** -- G-buffer generation shader. Vertex stage reuses `#include <include/vert.glsl>`. Fragment stage writes to multiple render targets (position, normal, albedo, metallic-roughness) instead of computing lighting. Handles alpha mask discard via existing `u_alpha_mode`/`u_alpha_cutoff` uniforms.
- **`deferred_lighting.glsl`** -- Deferred lighting shader. Vertex stage uses `#include <include/quad.glsl>`. Fragment stage samples G-buffer textures, then computes lighting reusing functions from `include/lighting.glsl` (`calculate_pbr_illumination`, `phone_shading`, `get_shadow`).

Existing shared shader headers (`include/uniforms.glsl`, `include/lighting.glsl`, `include/vert.glsl`, `include/quad.glsl`) are reused without modification.

### 6. Pipeline Mode Selection

Add a `render_mode` field to `GlobalSettings` (`global.h`) to switch between forward and deferred at runtime. The system that owns the renderer (likely the editor or application layer) instantiates the appropriate renderer based on this setting.

## Capabilities

### New Capabilities
- `deferred-rendering`: Render pipeline that generates a G-buffer and applies lighting in a separate fullscreen pass, enabling efficient multi-light scenes.

### Modified Capabilities
- `renderer-shared-infrastructure`: Extracted common rendering utilities (culling, shadows, velocity, TAA, bloom, post-process) usable by any renderer implementation.

## File Organization (After Change)

```
engine/runtime/source/render/
  renderer/
    renderer_forward.h/.cpp      -- forward-specific main pass + draw() orchestration
    renderer_deferred.h/.cpp     -- deferred G-buffer + lighting + forward transparency
  render_utils.h/.cpp            -- frustum culling, draw list, halton
  render_shared.h/.cpp           -- shared passes (shadow, velocity, TAA, bloom, postprocess), shared resources
  render_graph.h/.cpp            -- unchanged
  ...

engine/content/shader/
  gbuffer.glsl                   -- G-buffer generation
  deferred_lighting.glsl         -- deferred lighting pass
  include/
    lighting.glsl               -- unchanged, reused by deferred_lighting.glsl
    ...                          -- unchanged
```

## Impact

- **`renderer_forward.h/.cpp`**: Significant refactor -- shared code extracted out, but behavior unchanged.
- **`image.h` + `rhi/opengl_image.cpp` + `rhi/opengl_framebuffer.cpp`**: New `ImageFormat` entries (`RGB16F`, `RG16F`).
- **`global.h`**: New `render_mode` field.
- **Existing shaders**: Unchanged. Existing materials continue to work with forward rendering.
- **Memory**: G-buffer adds ~30 bytes/pixel of VRAM (position 6B + normal 6B + albedo 4B + metallic-roughness 4B + depth 4B at 1080p ~60MB).

## Non-goals

- Tile-based or clustered deferred rendering (future optimization for very high light counts).
- Screen-space reflections, SSAO, or other deferred-enabled effects (future work that builds on the G-buffer).
- Changing the material/shader authoring format -- existing `.glsl` shaders with `@uniforms`/`@stage` sections remain as-is.

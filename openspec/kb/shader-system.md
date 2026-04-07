# Shader System
> Summary: GLSL shader authoring, variant system, include mechanism, and validation
> Scope: engine/content/shader/, engine/runtime/source/render/shader*.h, engine/tool/

## Shader Files

- Location: `engine/content/shader/`
- Format: `.glsl`
- Include dir: `engine/content/shader/include/`

## Shader Inventory

### Main Shaders

| Shader | Purpose |
|--------|---------|
| `gbuffer.glsl` | G-buffer fill (deferred geometry pass) |
| `deferred_lighting.glsl` | Deferred lighting resolve |
| `deferred_skybox.glsl` | Skybox in deferred pipeline |
| `pbr.glsl` | PBR material (metallic-roughness) |
| `pbr_sg.glsl` | PBR specular-glossiness variant |
| `phone.glsl` | Phong/Blinn lighting |
| `unlit.glsl` | Unlit material |
| `shadow.glsl` | Shadow map generation |
| `bloom_downsample.glsl` | Bloom downsample pass |
| `bloom_upsample.glsl` | Bloom upsample pass |
| `postprocessing.glsl` | Tone mapping and final post-processing |
| `taa.glsl` | Temporal anti-aliasing |
| `velocity.glsl` | Velocity buffer for TAA |
| `picking.glsl` | Entity picking (mouse selection) |
| `picking_sprite.glsl` | Sprite picking |
| `sprite_2d.glsl` | 2D sprite rendering |
| `sprite_2d_batched.glsl` | Batched 2D sprites |
| `skybox.glsl` | Skybox rendering |
| `copy.glsl` | Fullscreen copy/blit |

### Include Files (`include/`)

- `uniforms.glsl` -- shared uniform declarations
- `vert.glsl` -- common vertex shader logic
- `frag_attrs.glsl` -- fragment attribute definitions
- `lighting.glsl` -- lighting calculations
- `pbr_frag.glsl`, `pbr_uniforms.glsl`, `pbr_reflections.glsl` -- PBR fragments
- `gbuffer_out.glsl`, `gbuffer_out_phone.glsl`, `gbuffer_out_sg.glsl`, `gbuffer_out_unlit.glsl` -- G-buffer output variants
- `shadow_out.glsl`, `shadow_out_sg.glsl` -- shadow output variants
- `velocity_out.glsl` -- velocity buffer output
- `quad.glsl` -- fullscreen quad vertex shader
- `reflections.glsl` -- reflection calculations

## Shader Variant System

- Key types: `Shader`, `ShaderModule`, `ShaderVariant` (in `render/shader.h`, `render/shader_variant.h`)
- `Shader::create(filepath)` loads and compiles a `.glsl` file
- `ShaderModule` represents a single stage (vertex, fragment)
- Variants allow different permutations of the same shader (e.g., with/without shadows)

## Validation

- Tool: `shader_validator.exe` (built from `engine/tool/`)
- Run via: `python dev/z1.py validate-shaders`
- Validates all `.glsl` files under `engine/content/shader/`

-> see [render-pipeline.md]
-> see [build.md]

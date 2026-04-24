## Why

Objects with no assigned material render visibly brighter in the forward pipeline than in the deferred pipeline. The cause is a lighting-model mismatch: the forward path uses Phong shading (`phone_shading`), while the deferred path resolves lighting with Cook-Torrance PBR (`calculate_pbr_illumination`). Phong omits the `/ PI` energy-conservation term, making diffuse contribution ~3× higher than the PBR result for the same scene illumination.

## What Changes

- Replace the `phone_shading` call in `phone.glsl`'s forward fragment stage with `calculate_pbr_illumination` using the same parameters (metallic = 0.0, roughness = 0.5) already written to the G-buffer by `gbuffer_out_phone.glsl`.
- Remove the now-unused `phone_shading` function from `lighting.glsl` (or demote it to a private helper) to prevent future divergence.

## Capabilities

### New Capabilities

- `default-material-lighting-parity`: Forward and deferred pipelines produce visually consistent brightness and shading for objects with no assigned material (the `MI_phone` default). Both paths use Cook-Torrance PBR with the same metallic/roughness constants.

### Modified Capabilities

<!-- No existing spec-level requirements change -->

## Impact

- `engine/content/shader/phone.glsl` — forward fragment stage switches from `phone_shading` to `calculate_pbr_illumination`.
- `engine/content/shader/include/gbuffer_out_phone.glsl` — no change; metallic/roughness constants are already the authoritative source.
- `engine/content/shader/include/lighting.glsl` — `phone_shading` either removed or left but no longer called on the default material path.
- Visual output: slightly darker forward-rendered default-material objects (matching deferred). All existing materials with explicit PBR shaders are unaffected.

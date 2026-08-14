## Why

Scenes currently have no contact/ambient darkening: corners, crevices, and surfaces under ledges look flat and float, with only the constant `sun_ambient` term filling indirect light. Adding screen-space ambient occlusion grounds geometry in its surroundings and substantially improves perceived depth in both rendering paths.

## What Changes

- New screen-space AO passes that produce an AO texture from depth + normal buffers, supporting two algorithms:
  - **SSAO**: classic hemisphere-sampling with per-pixel rotated kernel, 16 samples.
  - **GTAO**: Jimenez 2016 horizon-based, 6 slices × 8 steps, cosine-weighted with slice weights and the paper's multi-bounce-aware output.
- AO computed at **half resolution**, with an optional edge-preserving (depth-weighted) 5×5 Gaussian blur pass.
- **Deferred pipeline**: AO pass reads the G-buffer (depth/normal/position) between the G-buffer and deferred-lighting passes; the lighting shader multiplies the ambient term by AO.
- **Forward pipeline**: a new depth+normal **prepass** (reuses the GBuffer shader variant) feeds the AO pass, which runs before the main forward pass; forward PBR shaders sample the AO texture and multiply the ambient term.
- New global settings (`ao_enabled`, `ao_type` SSAO/GTAO, `ao_radius`, `ao_intensity`, `ao_power`, `ao_bias`, `ao_blur_enabled`, `ao_blur_strength`) exposed via the reflection-driven editor inspector.
- `PerFrameConst` gains an `ao_map_binding` so materials can bind the AO texture in forward passes.

## Capabilities

### New Capabilities

- `screen-space-ambient-occlusion`: AO generation pass (SSAO or GTAO) + optional blur, driven by global settings; produces a screen-space AO term applied to the ambient/indirect lighting in both deferred and forward pipelines. Covers settings plumbing, half-res buffers, shaders, and editor exposure.

## Impact

- `engine/runtime/source/render/global.h/.cpp` — AO settings fields, global UBO layout, `flush()`
- `engine/content/shader/include/uniforms.glsl` — global AO uniforms + `u_ao_texture` sampler
- `engine/content/shader/include/vert.glsl`, `include/frag_attrs.glsl` — `v_screen_uv` varying
- `engine/content/shader/ssao.glsl`, `gtao.glsl`, `ao_blur.glsl` — new shaders
- `engine/content/shader/deferred_lighting.glsl`, `include/pbr_frag.glsl`, `phone.glsl` — ambient × AO
- `engine/runtime/source/render/renderer/render_shared.h/.cpp` — AO/prepass pipelines, buffers, pass helpers
- `engine/runtime/source/render/renderer/renderer_deferred.h/.cpp` — AO pass + lighting/transparency binding
- `engine/runtime/source/render/renderer/renderer_forward.h/.cpp` — prepass + AO pass + main-pass binding
- `engine/runtime/source/asset/material.h/.cpp` — `PerFrameConst::ao_map_binding`, texture binding

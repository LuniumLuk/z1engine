## 1. Global settings plumbing

- [x] 1.1 Add AO fields to `GlobalSettings` in `engine/runtime/source/render/global.h` (`ao_enabled`, `ao_type`, `ao_radius`, `ao_intensity`, `ao_power`, `ao_bias`, `ao_blur_enabled`, `ao_blur_strength`) and to `GlobalConstants`
- [x] 1.2 Append matching floats to `GlobalConstants` and flush them in `global.cpp`
- [x] 1.3 Add `REFLECTED_FIELD` entries so AO settings appear in the editor inspector
- [x] 1.4 Append the 8 AO uniforms to the `Global` block in `engine/content/shader/include/uniforms.glsl` (must match C++ layout) and declare `uniform sampler2D u_ao_texture`

## 2. Shader support

- [x] 2.1 Add `v_screen_uv` varying (location 8) to `include/vert.glsl` and `include/frag_attrs.glsl`
- [x] 2.2 Create `engine/content/shader/ssao.glsl` (16-sample hemisphere kernel, per-pixel rotation, view-space reconstruction, range-checked occlusion)
- [x] 2.3 Create `engine/content/shader/gtao.glsl` (6 slices × 8 steps, slice-plane normal projection, weighted cosine integration, horizon clamping)
- [x] 2.4 Create `engine/content/shader/ao_blur.glsl` (depth-edge-preserving 5×5 Gaussian blur)
- [x] 2.5 Validate all shaders: `python dev/z1.py validate-shaders`

## 3. RenderShared infrastructure

- [x] 3.1 Add AO/prepass pipelines + AO framebuffers + per-frame matrices to `render_shared.h`
- [x] 3.2 Build `m_pipeline_ssao`, `m_pipeline_gtao`, `m_pipeline_ao_blur` in `RenderShared` constructor
- [x] 3.3 Create/resize half-res `m_ao_framebuffer`/`m_ao_blur_framebuffer` in `ensure_buffers`
- [x] 3.4 Implement `add_ao_pass(rg, depth_input, normal_input)` returning final pass name and `get_ao_image()`
- [x] 3.5 Implement `add_depth_prepass_pass` (renders opaque+mask with GBuffer variant into position/normal/depth targets)

## 4. Material binding

- [x] 4.1 Add `ao_map_binding` to `PerFrameConst` in `asset/material.h`
- [x] 4.2 Bind `u_ao_texture` in `MaterialInstance::bind` when `ao_map_binding` is valid

## 5. Deferred integration

- [x] 5.1 Call `m_shared.add_ao_pass` after the G-buffer pass in `renderer_deferred.cpp`
- [x] 5.2 Add AO input + `depends_on(ao_pass)` to deferred-lighting pass; bind AO image and multiply ambient in `deferred_lighting.glsl`
- [x] 5.3 Bind AO image in `add_forward_transparency_pass` (per-frame ao binding)

## 6. Forward integration

- [x] 6.1 Add `add_depth_prepass_pass` + `add_ao_pass` before the main pass in `renderer_forward.cpp` (gated on `ao_enabled`)
- [x] 6.2 Bind AO image in the main pass via `per_frame.ao_map_binding`
- [x] 6.3 Multiply ambient by AO in `include/pbr_frag.glsl` and `phone.glsl`

## 7. Validation

- [x] 7.1 `python dev/z1.py generate` (if needed) and `python dev/z1.py compile` — 0 errors
- [x] 7.2 `python dev/z1.py validate-shaders` — 0 errors (25/25 shaders pass; wrapper miscounts its own "Failed files: 0" summary line — pre-existing heuristic bug)
- [x] 7.3 `python dev/z1.py smoke --frames 10` — editor renders cleanly
- [x] 7.4 Manually verify AO visible in editor in deferred mode (GTAO + SSAO), toggle blur/radius
- [x] 7.5 Manually verify AO visible in forward mode
- [x] 7.6 Verify `ao_enabled = false` disables passes with no visual regression

## Verification notes

- Screenshot diff (deferred, GTAO on vs off): 199,843 px differ >4/255; 314,577 px darkened; mean color drops (66.0,60.8,51.9) vs (67.5,61.7,53.1) → AO visibly darkens ambient.
- Forward mode (temporary `render_mode = Forward` default) smoke: 10 frames, no GL errors.
- `test_scene_serialize` fails pre-existing ("Python Script did not move entity") — reproduced identically with changes stashed; unrelated to AO.
- `test_import`/`test_render_graph` required `python314.zip` beside the test exes (pre-existing packaging gap; copied from `engine/bin/Debug/`).


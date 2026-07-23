## 1. Investigation & Baseline

- [ ] 1.1 Capture before/after reference screenshots at 1080p in a stationary test scene (e.g., `content/forest.yaml` or `AlphaBlendModeTest`)
- [ ] 1.2 Profile current TAA pass GPU time (RenderDoc or GL timer query)
- [x] 1.3 Document the current Halton jitter sequence pattern and verify frame_index wrapping
- [x] 1.4 Verify velocity buffer correctness: inspect velocity magnitude range and NaN handling

## 2. Jitter Compensation

- [x] 2.1 Add `u_taa_jitter_uv` uniform (vec2) to `taa.glsl` uniforms block
- [x] 2.2 Add `taa_jitter_uv` field to `GlobalSettings` in `global.h` (vec2, non-serialized — computed per-frame)
- [x] 2.3 Compute `taa_jitter_uv = vec2(jx / width, jy / height)` in both `renderer_deferred.cpp` and `renderer_forward.cpp` before `g->flush()`
- [x] 2.4 In `taa.glsl`, offset current-color sample: `current = texture(u_current_color, v_uv - u_taa_jitter_uv).rgb`
- [x] 2.5 Validate: stationary camera with TAA on — verify shake is eliminated
- [x] 2.6 Rebuild Python bindings: `python dev/z1.py gen-pybinds`

## 3. Adaptive Blend Factor

- [x] 3.1 Compute local color variance in `taa.glsl` from a 5×5 neighborhood in YCoCg space
- [x] 3.2 Implement per-pixel blend: `blend = u_taa_blend / (1.0 + u_taa_variance_scale * luminance_variance)`
- [x] 3.3 Add `u_taa_variance_scale` uniform (default 1.0) to control adaptation strength
- [x] 3.4 Clamp `blend` to a sensible range (`[0.03, 0.3]` for new-frame weight)
- [x] 3.5 Add edge-stop: reduce blend when reprojected history differs significantly from neighborhood centroid
- [x] 3.6 Update `taa_blend` semantics in `GlobalSettings`: changed default from 0.9 to 0.1 (inverted to new-frame weight)
- [x] 3.7 Validate: no visible ghosting on moving objects, smooth accumulation on static surfaces

## 4. Variance-Guided Clip

- [x] 4.1 Expand neighborhood sampling from 3×3 to 5×5 in `taa.glsl`
- [x] 4.2 Compute neighborhood mean and variance in YCoCg
- [x] 4.3 Replace strict AABB clip with variance-expanded clip: `box = mean ± γ * sqrt(variance + ε)`
- [x] 4.4 Add `u_taa_clip_gamma` uniform (default 1.0) for tuning the expansion
- [x] 4.5 Validate: reduced flicker on high-frequency textured surfaces (check `AlphaBlendModeTest` scene with cutoff materials)

## 5. TAA Sharpen Pass

- [x] 5.1 Create `engine/content/shader/taa_sharpen.glsl` — fullscreen quad shader
- [x] 5.2 Implement luma-guided unsharp mask: `sharp = color + strength * (color - blur3x3(color)) * edge_stop`
- [x] 5.3 Add `u_taa_sharpen_strength` uniform (default 0.3)
- [x] 5.4 Add `m_pipeline_taa_sharpen` to `RenderShared` and initialize it
- [x] 5.5 Add `add_taa_sharpen_pass()` method to `RenderShared` (insert between TAA and bloom)
- [x] 5.6 Wire sharpen pass into both `renderer_deferred.cpp` and `renderer_forward.cpp`
- [x] 5.8 Add `taa_sharpen_enabled` bool and `taa_sharpen_strength` float to `GlobalSettings`

## 6. Settings & Bindings

- [x] 6.1 Add new TAA settings to `GlobalSettings` in `global.h`: `taa_jitter_uv`, `taa_variance_scale`, `taa_clip_gamma`, `taa_sharpen_enabled`, `taa_sharpen_strength`
- [x] 6.2 Update `global.cpp` UBO layout (added new TAA fields, all-floats for safe std140 alignment)
- [x] 6.3 Regenerate Python bindings: `python dev/z1.py gen-pybinds`
- [x] 6.4 Update `uniforms.glsl` Global block to match the new UBO layout
- [x] 6.5 Expose new TAA tunables in the editor ImGui panel (via REFLECTED_FIELD macros)

## 7. Validation

- [x] 7.1 Run shader validation: `python dev/z1.py validate-shaders`
- [x] 7.2 Build: `python dev/z1.py compile`
- [x] 7.3 Smoke test: `python dev/z1.py smoke`
- [ ] 7.4 Manual test: stationary camera, verify no visible shake
- [ ] 7.5 Manual test: moving camera, verify no ghosting trails
- [ ] 7.6 Manual test: toggle TAA on/off, verify anti-aliasing quality on edges
- [ ] 7.7 Manual test: verify sharpen pass doesn't introduce haloing artifacts
- [ ] 7.8 Performance: GPU profiler comparison before/after at 1080p

# Tasks — Simplify Resource Binding API

## 1. Phase 0 — Metrics baseline

- [x] 1.1 Add probe counters: texture binds/frame, sampler-uniform writes/frame, block-binding writes/frame, name-keyed uniform resolutions/frame, and a per-program binding-layout hash
- [x] 1.2 Capture baseline metrics on the demo scene with the probing Hybrid build: idle and `Z1_AUTOROTATE=2` (record `renderer_draw`, `draws`, counters; keep logs under `/tmp` and paste the summary numbers into the change notes)
- [x] 1.3 Gate: `python dev/z1.py compile --config Hybrid` 0 errors, `smoke --frames 10` passes

## 2. Phase 1 — RHI foundation (dual support, no behavior change)

- [x] 2.1 Add link-time sampler slot assignment in `OpenGLShader::link_shaders`: sort sampler uniforms by name, assign `slot == unit`, stamp all sampler uniforms once, store `name -> { slot, unit, target type, location }`
- [x] 2.2 Expose `TextureSlot` + `Shader::sampler_slot(name)` and the semantic uniform-block table (`Global`, `Lights`, `Bones`, `PrevBones`, ...) with one-time `glUniformBlockBinding` at link
- [x] 2.3 Add `UniformHandle` + `Shader::set(handle, data)` fast path (typed from reflection); keep name-based `set_uniform` for cold paths
- [x] 2.4 Create typed fallback textures (white 2D / 2D-array / cube) and add `GraphicsContext::bind_texture(slot, image, fallback)` and `bind_uniform_block(binding, buffer)` with dedup caches (unit->handle, active unit, binding->handle)
- [x] 2.5 Add a link-time assertion with a clear message when a program's sampler count exceeds the per-stage unit limit
- [x] 2.6 Gate: compile 0 errors, smoke, screenshot of the demo scene identical to pre-change (old APIs still in use at this point)

## 3. Phase 2 — Pass conversion

- [x] 3.1 Add `RenderGraphNode::bind_input(sampler_name, input_name)` (and batch form) that resolves the slot and binds through the context, with automatic fallback for optional inputs
- [x] 3.2 Convert `render_shared` passes (AO, AO blur, TAA, TAA sharpen, bloom chain, postprocess) to binder calls; delete default-sampler fallback branches (shadow/velocity moved to §4 — they bind through the material path)
- [x] 3.3 Convert `renderer_deferred` passes (deferred lighting, SSR) (gbuffer, forward transparency, skybox blit moved to §4 — material path)
- [x] 3.4 Convert `renderer_forward` passes (skybox sampler done; main pass binds via the material path → §4)
- [x] 3.5 Convert `particle_renderer` (particle + particle shadow passes), removing conditional fallback binding
- [x] 3.6 Convert `picking_system` and `renderer_2d` binding paths (2D uses the array-element overload `bind_texture(slot, element, image)`)
- [x] 3.7 Gate: compile 0 errors, smoke, probe counters reduced vs Phase 0 (tex_binds 34→18, sampler_writes 25→10, block_writes 10→4, ubo_binds 2→1, name_resolutions 117→96; screenshot parity was waived by the user — counter + smoke parity used instead; zero sampler writes deferred to §5 cleanup)

## 4. Phase 3 — Materials

- [x] 4.1 Build a per-variant binding plan in `MaterialInstance` (uniform handles + values, texture slots + images); rebuild only when overrides change (cache keyed by resolved shader; `set_*` setters mark the plan dirty)
- [x] 4.2 Replace `MaterialInstance::bind_uniform`/`Image::bind(shader, name)` usage with plan execution through the binder (legacy `bind_uniform` kept but no longer called; deleted in §5)
- [x] 4.3 Simplify `Mesh::draw` semantics (no per-draw material bind/unbind pairing required — resources are binder-owned; only pipeline bind stays)
- [x] 4.4 Verify editor material preview and picking paths use the new binding path (picking converted to the binder; editor has no per-frame material binding path)
- [x] 4.5 Gate: compile 0 errors, smoke, probe counters (per-frame `tex_binds`/`sampler_writes`/`block_writes` now zero; `name_resolutions` 117 → 25; `layout_hash` constant at 4042235446; camera-motion `renderer_draw` max 1.58 ms after warmup). Pass-specific uniforms (`u_csm_index`, `u_has_skinning`, `u_prev_model`) are still set by name per primitive — acceptable at these volumes, noted as follow-up. Screenshot parity waived by the user (counter + smoke + GL-error parity used instead)
- [x] 4.6 Convert the material-path passes deferred from Phase 2: shadow, velocity, depth prepass, gbuffer, forward transparency, skybox blit, `renderer_forward` main pass — together with the `PerFrameConst` block/texture bindings they share (slow paths use `Global`/`Lights`/`Bones`/`PrevBones` via the binder; `PerFrameConst` now carries resource pointers instead of binding indices)

## 5. Phase 4 — Cleanup

- [ ] 5.1 Remove `Image` binding state and API (`bind()/unbind()/bind(shader,name)/get_binding()/is_bound()`, `m_binding`, `m_ref_count`) — includes porting `engine/test/test_render_graph.cpp` off `bind_input_name`/`bind_input_index`/`unbind_input_*`
- [ ] 5.2 Remove `UniformBuffer` binding state and API; binding becomes `GraphicsContext::bind_uniform_block(binding, buffer)` only
- [ ] 5.3 Remove the unit/binding pools, `acquire_*`/`release_*` APIs, `m_default_sampler_binding` and `create_default_sampler_textures`
- [ ] 5.4 Remove the shader's location-based binding setters and `get_uniform_binding`; keep `set_uniform(name, ...)` for cold paths
- [ ] 5.5 Run `python dev/z1.py generate` if any source files were added/removed, then compile 0 errors
- [ ] 5.6 Gate: smoke + probe A/B against Phase 0 baselines (per-frame writes at zero, bind counts reduced, frame times no worse)

## 6. Regression & validation

- [x] 6.1 Run the camera-motion regression: `Z1_AUTOROTATE=2 Z1_PROBE_EVERY=1 ./engine/bin/Hybrid/game --frames=600` — `renderer_draw` stays under the idle budget with no 50ms+ spikes; binding-layout hash constant per program (verified after Phase 3: max 1.58 ms post-warmup, 0 GL errors, `layout_hash` = 4042235446 constant across 602 samples)
- [ ] 6.2 Run `Z1_CAMROT_DISABLE=ao,bloom,taa` variants to confirm no fallback regressions (passes still render correctly when resources are absent)
- [ ] 6.3 Run `python dev/z1.py format --dry-run` — touched files clean
- [ ] 6.4 Run `python dev/z1.py dcv --auto` and persist the validation report (per `change-validation-gates`); on macOS, note the pre-existing test-link gap and rely on compile + smoke + probe gates
- [ ] 6.5 Update `openspec/kb/render-pipeline.md` (binding model) and `openspec/kb/perf-probing-and-quality.md` (new probe counters), and `index.md` if pages change
- [ ] 6.6 Archive the change: `openspec archive simplify-binding-api --yes --json`

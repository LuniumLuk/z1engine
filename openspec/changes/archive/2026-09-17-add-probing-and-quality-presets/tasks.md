## 1. Probing formalization

- [x] 1.1 Create `engine/runtime/source/util/prober.h` with the `ENABLE_PROBING` gate and the public macros
      (`PROBE_CONFIGURE`, `PROBE_FRAME_BEGIN/END`, `PROBE_SCOPE`, `PROBE_VALUE`, `PROBE_GPU_BEGIN/END_FRAME`,
      `PROBE_PRINT_SUMMARY`), compiling to no-ops when disabled
- [x] 1.2 Create `engine/runtime/source/util/prober.cpp`: scope accumulators, periodic reports and session
      summary (via `CORE_INFO`), env configuration (`Z1_PROBE_EVERY`, `Z1_PROBE_QUIET`, `Z1_PROBE_VSYNC`,
      `Z1_PROBE_FINISH`, `Z1_PROBE_NO_GPU`), one-time environment info logged after the first frame
- [x] 1.3 Implement the GPU timer-query ring in `prober.cpp` (create/resolve `GL_TIME_ELAPSED` queries,
      non-blocking availability check, miss counting, optional timed `glFinish` drain)
- [x] 1.4 Define `ENABLE_PROBING` for the `Hybrid` configuration in `premake5.lua` (workspace level)
- [x] 1.5 Replace the `[TEMP-PROBE]` code everywhere with the new macros/scopes: `application.cpp`,
      `window.cpp`, `imgui_layer.cpp`, `editor_layer.cpp`, `opengl_context.cpp` (GPU hooks; keep
      `Z1_NO_GL_CHECK` under `ENABLE_PROBING`)
- [x] 1.6 Delete `engine/runtime/source/util/frame_probe.h`, the removed experiment knobs and the game-mode
      camera injection (`game.cpp`); keep behavior identical without probing
- [x] 1.7 Regenerate projects (`python3 dev/z1.py generate`) and build Hybrid (`compile`) with 0 errors
      (verified: `RESULT: ok, 0 errors`; probe output verified in the smoke run)

## 2. Shadow cascade configuration

- [x] 2.1 Add `AOResolution`, `ShadowResolution`, `ShadowCascades` enums (with `REFLECT_ENUM`) and the
      `ao_resolution`, `sm_resolution`, `sm_cascade_count` fields to `GlobalSettings` in `render/global.h`,
      with `REFLECTED_FIELD` entries (shadow / ambient_occlusion groups)
- [x] 2.2 Append `csm_cascade_count` to the `Global` UBO mirror in `global.cpp`/`global.h` and
      `shader/include/uniforms.glsl`; update the std140 size static assert (768 → 784, the `alignas(16)`
      struct rounds the trailing int up) and `flush()`
- [x] 2.3 Generalize `RenderShared::calculate_csm_splits` to N cascades (split computation, `csm_splits`
      padding, valid `sun_projview` slots); move shadow framebuffer creation into
      `ensure_shadow_resources()` (resolution + layer count, recreate on change, refresh `m_shadow_image`;
      falls back to the defaults when called from the ctor, before `GlobalSettings` exists)
- [x] 2.4 Make `add_shadow_pass` and the particle shadow passes loop the configured cascade count; update
      `renderer_deferred.cpp` / `renderer_forward.cpp` call sites to pass it
- [x] 2.5 Update shader cascade selection: `u_csm_cascade_count` guard in `include/lighting.glsl`
      (`get_cascade_index`) and `particle.glsl`; clamp beyond-range distances to the last valid cascade
- [x] 2.6 Wire `ao_resolution` into `RenderShared::ensure_buffers` (half or quarter divisor)
- [x] 2.7 Gate: `validate-shaders` (187 passed; the single failure is the pre-existing
      `sprite_2d_batched.glsl` Apple 16-sampler limit, unrelated to this change), `compile` (0 errors) and
      `test --filter test_render_graph` — test executables cannot be linked on macOS yet (pre-existing
      `_glad_*` link gap, `engine/bin/test/` empty before this change), so the render-graph test could not run

## 3. Render-graph pass skipping (so presets save real work)

- [x] 3.1 Parameterize `add_bloom_pass(rg, input_name)` and
      `add_postprocess_pass(rg, target, scene_input_name, bloom_present)` in `render_shared.{h,cpp}`
- [x] 3.2 Deferred renderer: build the final-color chain — skip velocity/TAA/sharpen when `taa_enabled` is
      false, skip bloom when `pp_bloom_enabled` is false, feed postprocess accordingly
- [x] 3.3 Forward renderer: same wiring as 3.2
- [x] 3.4 Gate: `compile` (0 errors) and `smoke --frames 10` (editor starts, renders, exits cleanly)

## 3a. RHI fix found during validation (1-cascade shadow map)

- [x] 3a.1 `Framebuffer::Attachment` gains a `layered` flag; `OpenGLFramebuffer::create()`/
      `bind_attachment()` treat `layers > 1 || layered` as an array; the shadow attachment sets it
- [x] 3a.2 Relax `OpenGLImage2DArray`'s `m_depth > 1` assert to `>= 1` (single-layer arrays are valid GL)
- [x] 3a.3 Verified: LOW (1 cascade) went from 802 `GL_INVALID_OPERATION` to 0 with identical performance
      (bisected: the same preset with 2 cascades was already clean)

## 4. Editor quality presets

- [x] 4.1 Create `engine/editor/source/quality_preset.{h,cpp}` with `QualityPreset`, display names and
      `apply_quality_preset(GlobalSettings&, QualityPreset)` implementing the LOW/MEDIUM/HIGH table
- [x] 4.2 Add `quality_preset` to `EditorSettings` (load/save with default HIGH)
- [x] 4.3 Apply the stored preset in `EditorLayer` after the initial scene load; add the selector combo
      (dedicated function) to the top of the global settings panel; apply on selection change
- [x] 4.4 Regenerate projects (new editor files) and `compile` with 0 errors
- [x] 4.5 Gate: `smoke --frames 10`; A/B probing runs confirmed the presets work — HIGH ≈ 91 fps steady /
      GPU ≈ 11.8 ms, LOW ≈ 120 fps (present floor) / GPU ≈ 5.3 ms at the default 640×480 viewport, and
      screenshot runs of both presets render the helmet scene correctly (shadows, AO, materials)

## 5. macOS robustness fixes

- [x] 5.1 Apply the window's advertised vsync state in `OpenGLContext::init()` via `glfwSwapInterval`
- [x] 5.2 Add `Window::is_display_available()` (monitor count > 0) and skip layer/ImGui updates in
      `Application::run()` while no display is present (no ImGui monitor-list assert, resumes on wake)
- [x] 5.3 Gate: `compile` (0 errors) and `smoke --frames 10`

## 6. Verification and documentation

- [x] 6.1 Run `python3 dev/z1.py generate` then full `compile` for the affected configs on this platform
      (`Hybrid`, 0 errors; `format --dry-run` reports only 2 pre-existing files)
- [x] 6.2 Run the test suites applicable to the touched paths and `validate-shaders` — shaders: only the
      pre-existing `sprite_2d_batched.glsl` sampler-limit failure; test executables cannot be built on macOS
      yet (pre-existing link gap), so suites ran only to the extent the platform allows
- [x] 6.3 Run the editor smoke test and probing runs: `[probe]` environment line, scopes, GPU queries and the
      session summary all report correctly, and LOW vs HIGH frame times/GPU times match expectations
- [x] 6.4 Update `docs/PROFILING_MACOS_OPENGL.md` (formalized `ENABLE_PROBING` section, new knobs,
      reproduction commands, §8 follow-up summary) and note the preset feature
- [x] 6.5 Update `openspec/kb/`: new `perf-probing-and-quality.md` page (+ index entry) and
      `render-pipeline.md` (cascade settings, AO resolution, conditional TAA/bloom passes)

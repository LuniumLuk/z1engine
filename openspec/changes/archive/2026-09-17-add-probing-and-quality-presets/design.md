## Context

The engine has two timing facilities today:

- `ENABLE_PROFILE` (`Profile` config) → `Instrumentor` chrome-trace JSON, CPU-side only, always-compiled when
  configured (see `util/instrumentor.h`, `PROFILE_*` macros).
- A temporary `[TEMP-PROBE]` frame probe added during the 2026-09-17 macOS profiling session
  (`util/frame_probe.h` + call sites in `application.cpp`, `window.cpp`, `imgui_layer.cpp`,
  `editor_layer.cpp`, `game.cpp`, `render_shared.cpp`, `opengl_context.{h,cpp}`) — prints `[probe]` reports
  but is unconditional, ad-hoc, and carries experiment knobs (`Z1_PROBE_SKIP_RENDER`, `Z1_PROBE_NO_AO`, …).

Rendering settings live in `GlobalSettings` (`render/global.h`), reflected and serialized per scene, mirrored
into the `Global` UBO (`GlobalConstants`, std140-checked, layout must match `shader/include/uniforms.glsl`).
Shadow cascades are hard-coded (`CSM_LAYERS 4`, 2048²) in `RenderShared`; AO is hard-coded to half resolution.
The editor draws reflected global settings in its "settings" panel via `show_type_fields`. Game mode
(`GameApp`) links only runtime + editor-less code; game developers configure settings from scripts.

The parsing of the macOS session results (`docs/PROFILING_MACOS_OPENGL.md`) identified the exact problems this
change addresses, including two platform bugs and two missing levers (probing, quality presets).

## Goals / Non-Goals

**Goals:**

- One formal probing module: compile-time gated (`ENABLE_PROBING`), CPU scopes **and** GPU frame time, periodic
  reports + run summary, safe on the macOS GL driver.
- Editor-only LOW/MEDIUM/HIGH quality presets that set real global settings, with persistence and a dedicated
  UI + apply function; they must remove real GPU work (skip TAA/velocity/bloom passes when off).
- Configurable shadow map resolution and cascade count (1–4) end-to-end (settings → renderer → shaders).
- Fix: vsync state actually applied at startup; render loop survives a sleeping display without crashing.

**Non-Goals:**

- No change to the chrome-trace profiling path (`ENABLE_PROFILE` stays as-is).
- No quality presets (or any quality policy) in game mode — games set `GlobalSettings` from scripts.
- No temporal reprojection for AO, no new rendering features.
- No editor UI redesign beyond the preset selector row in the existing settings panel.

## Decisions

### D1. Probing = `util/prober.{h,cpp}` gated by `ENABLE_PROBING`, CPU+GPU, macro-based

- New `z1::prober` module with macros that compile to nothing unless `ENABLE_PROBING` is defined:
  `PROBE_CONFIGURE()`, `PROBE_FRAME_BEGIN()/PROBE_FRAME_END()`, `PROBE_SCOPE(name)`, `PROBE_VALUE(name, v)`,
  `PROBE_GPU_BEGIN_FRAME()/PROBE_GPU_END_FRAME()`, `PROBE_PRINT_SUMMARY()`. Call sites keep the names used by
  the temporary probe (`frame`, `update`, `imgui`, `window_update`, `swap`, `draws`, plus editor/imgui
  sub-scopes) so reports remain comparable with the profiling report.
- The define is added at workspace level for the `Hybrid` config (`premake5.lua`), so runtime + editor + game
  agree. `Profile` keeps `ENABLE_PROFILE` only (CPU trace); the two are orthogonal and can coexist.
- Reports go through `CORE_INFO` (sanctioned logging) every `Z1_PROBE_EVERY` frames (default 120), with a
  final summary at shutdown.
- GPU timing: a 4-deep `GL_TIME_ELAPSED` query ring. The sample from frame N−2 is read back (availability
  checked, never blocking) and recorded as scope `gpu`. Alternate considered: blocking `glGetQueryObject`
  reads — rejected (serialises the pipeline). The macOS driver quirk (availability never flips until the
  stream syncs) is tolerated: unavailable samples are skipped and counted in the report. Diagnostic knobs:
  `Z1_PROBE_NO_GPU`, `Z1_PROBE_FINISH` (timed `glFinish()` before present), `Z1_PROBE_VSYNC` (swap-interval
  override), `Z1_NO_GL_CHECK` (error-polling A/B, read in `glCheckError_`).
- One-time context/window info is logged after the first frame (not at window creation — the framebuffer-size
  read immediately after `glfwCreateWindow` is transiently wrong on macOS).

### D2. Quality presets are editor code with a standalone apply function

- `engine/editor/source/quality_preset.{h,cpp}`:
  `enum struct QualityPreset : int { Low, Medium, High }` plus
  `void apply_quality_preset(GlobalSettings&, QualityPreset)` and a display-name helper. Presets are a
  **dedicated function**, not reflection: nothing is registered, nothing is serialized as part of scenes.
- Preset values (quality levers only; artistic values like AO radius/gamma are untouched):

  | Setting | LOW | MEDIUM | HIGH |
  |---|---|---|---|
  | `taa_enabled` / `taa_sharpen_enabled` | off / off | on / off | on / on |
  | `ao_enabled` / `ao_resolution` / `ao_blur_enabled` | on / quarter / on | on / half / on | on / half / on |
  | `pp_bloom_enabled` | off | on | on |
  | `ssr_enabled` | off | untouched | untouched |
  | `sm_resolution` / `sm_cascade_count` | 1024 / 1 | 2048 / 2 | 2048 / 4 |

- Persistence: `quality_preset` int in `EditorSettings` (`editor_settings.yaml`), default `High`. Applied in
  `EditorLayer` after the initial scene load and whenever the user changes the selector; **not** re-applied on
  every subsequent scene load (a scene's own settings stay authoritative until the user acts).
- UI: a combo + explanatory line at the top of `EditorLayer::show_settings()` (the global panel), rendered by
  a small dedicated function, followed by the existing reflected fields.
- Game mode: no preset code is linked into the game; the README of the module states the script obligation.

### D3. Shadow cascades + resolution are reflected global settings

- New enums in `render/global.h` with `REFLECT_ENUM` + `REFLECTED_FIELD`:
  `AOResolution { Half, Quarter }`, `ShadowResolution { Res512, Res1024, Res2048, Res4096 }`,
  `ShadowCascades { One = 1, Two, Three, Four }` (custom underlying values are supported by the enum
  registry; serialization is by name with int fallback).
- UBO: append `float csm_cascade_count` to `GlobalConstants` and `float u_csm_cascade_count;` to the `Global`
  block in `uniforms.glsl` (append-only to preserve the existing layout); update the `sizeof` static assert
  (768 → 784) and set the field in `GlobalSettings::flush()`.
- `RenderShared`:
  - Shadow framebuffer creation moves out of the constructor into `ensure_shadow_resources()` which recreates
    it only when `(resolution, cascade_count)` changed (cheap int compare per frame; one-time recreate spike
    on preset change is acceptable).
  - `calculate_csm_splits()` is generalized to N cascades: splits computed with the same log/uniform
    interpolation using `p = i / N`; `csm_splits = (s1, s2 or far, s3 or far, far)`; `sun_projview[i]` for
    `i < N`, remaining slots duplicate the last layer (never selected).
  - `add_shadow_pass()` and the particle shadow passes loop the configured cascade count;
    `renderer_deferred.cpp` / `renderer_forward.cpp` pass it through instead of `CSM_LAYERS`.
- Shaders: `include/lighting.glsl` `get_cascade_index()` and `particle.glsl`'s cascade pick gain
  `u_csm_cascade_count` guards:
  `if (count > 1 && dist >= splits.x) layer = 1; …` — with `count == 1` everything in range maps to layer 0,
  beyond the far split clamps to the last valid layer. Shadow array layers = cascade count, so out-of-range
  sampling is impossible by construction.
- `MAX_CSM_LAYERS` (4) replaces the `CSM_LAYERS` macro as the array bound; the runtime count comes from
  settings.

### D4. Presets must remove real work: conditional TAA/velocity/bloom wiring

Measured cost at 1080p: TAA + velocity ≈ 13–16 ms, bloom ≈ 2–4 ms. Today `taa_enabled` only disables camera
jitter while all passes still run; `pp_bloom_enabled` only stops the postprocess from *sampling* bloom. Fix in
both renderers:

- Build a `final_color` name: `scene-color` (→ `scene-color-ssr` when SSR on) when TAA is off, else run
  velocity + TAA + sharpen and use `taa-sharpen`.
- Add bloom passes only when `pp_bloom_enabled`; `add_bloom_pass(rg, input_name)` takes its source pass name.
- `add_postprocess_pass(rg, target, scene_input_name, bloom_present)` binds `bloom-up-1` only when bloom ran.
- Velocity pass is skipped whenever TAA is off (its only consumer).

### D5. macOS fixes

- **Vsync**: `OpenGLContext::init()` applies the window's advertised state
  (`glfwSwapInterval(is_v_sync_enabled() ? 1 : 0)`) right after the context becomes current. The
  `Z1_PROBE_VSYNC` override (probing) still wins because it runs later. *(Alternative: flip the default flag
  to false — rejected; the checkbox intent and the profiling recommendation are "vsync on by default".)*
- **Display sleep**: add `Window::is_display_available()` (true when `glfwGetMonitors` reports ≥ 1 monitor).
  `Application::run()` skips the layer/ImGui section when the display is unavailable, which prevents the
  ImGui multi-viewport assert (`Platform init didn't setup Monitors list`) and stops burning GPU while the
  screen is off; rendering resumes automatically on wake. *(Alternative: unset `ViewportsEnable` while no
  monitors — rejected: docking state churn and still-frame issues.)*
- The `Z1_NO_GL_CHECK` error-check A/B remains, now documented as a probing knob.

### D6. Temporary probe removal

`util/frame_probe.h` and every `[TEMP-PROBE]` call site are deleted; experiment-only knobs are dropped
(superseded by settings/presets). The game-mode camera injection is removed — `--game` on editor-authored
scenes requires an authored camera (documented; smoke/automation keeps the editor path).

### D7. Single-layer texture arrays in the RHI (found during validation)

The shadow map must stay a `sampler2DArray` even when `sm_cascade_count == 1`, but
`OpenGLFramebuffer::create()` built a plain `GL_TEXTURE_2D` for `layers <= 1`, so
`set_attachment_layer(0, 0)` raised `GL_INVALID_OPERATION` every frame (bisected: LOW=1 cascade failed,
the same preset with 2 cascades was clean). Fix: `Framebuffer::Attachment` gains a `layered` flag
(default `false`) that forces array creation for a single layer; `RenderShared::ensure_shadow_resources()`
sets it. Existing attachments are unaffected.

## Risks / Trade-offs

- [UBO layout drift vs shaders] → append-only field, `static_assert(sizeof(GlobalConstants) == 784)`, plus
  `validate-shaders` and `test_render_graph` gates; verification runs on both configured shader targets.
- [Cascade count < 4 crashes/artifacts from out-of-range layers] → shadow array layers equal the cascade
  count, selection clamped by the new uniform guards, and remaining `sun_projview` slots are valid
  duplicates.
- [Preset clobbering scene-tuned quality levers at startup] → presets touch only TAA/AO-resolution/bloom/
  shadows/SSR; HIGH matches previous defaults; applied at startup and on explicit selection only.
- [GPU timing unreliable on macOS] → non-blocking readback with skip + counter; `Z1_PROBE_FINISH` available
  when a synchronised number is needed; wall-clock deltas remain the trustworthy metric.
- [Display guard hides windows in unusual hot-plug states] → guard is re-evaluated every frame and clears as
  soon as a monitor is reported.
- [Extra per-frame checks] → probing compiles out when disabled; shadow resource check is two int compares.

## Migration Plan

No persisted-format breakage: new settings are additive (old scenes load with Half / 2048 / Four defaults,
i.e. current behavior). `editor_settings.yaml` gains `quality_preset` (absent → default High). The temporary
probe knobs disappear — any scripts using them should switch to the settings/presets or the new probing
knobs. Rollback: revert the change; no data migration involved.

## Open Questions

- None blocking. (Future: expose AO sample-count quality — not required for the current presets.)

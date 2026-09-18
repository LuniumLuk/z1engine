## Why

The macOS profiling session (2026-09-17, results in `docs/PROFILING_MACOS_OPENGL.md`) left four actionable
gaps:

1. Frame timing lived in temporary `[TEMP-PROBE]` code: useful, but ad-hoc, always compiled in, and blind to
   GPU cost. The engine needs a first-class, compile-time-gated frame prober that measures **CPU scopes and
   GPU time**.
2. There is no quality scaling. On weak GPUs (this Intel iGPU) users must hand-tune a dozen global settings
   to trade quality for frame rate; the editor should ship LOW/MEDIUM/HIGH presets.
3. Two platform problems were pinpointed: the advertised vsync state is never applied to the GL context
   (bookkeeping-only flag), and the render loop **crashes** (ImGui multi-viewport assert) when the display
   sleeps, while also pointlessly burning GPU.
4. Shadow cascades are hard-coded at 4 cascades × 2048² (fixed per-frame cost); they must be configurable
   (1–4 cascades, selectable resolution) both as a setting and for the quality presets.

## What Changes

- Add a formal probing system gated by a new compile option `ENABLE_PROBING` (enabled in the `Hybrid`
  config), replacing the temporary `[TEMP-PROBE]` instrumentation:
  - CPU wall-clock scopes (frame, update, imgui, swap, …) with periodic reports and a session summary.
  - GPU frame time via a `GL_TIME_ELAPSED` query ring, read back without stalling and tolerant of the
    macOS driver quirk where query availability never flips until the stream syncs.
  - Diagnostic knobs (report interval, quiet mode, vsync override, GPU sync probe, GL error-check A/B).
  - Distinct from the existing `ENABLE_PROFILE` (chrome-trace CPU instrumentation): probing is lightweight,
    budget-oriented, CPU+GPU.
- Add editor-only quality presets **LOW / MEDIUM / HIGH**:
  - Selectable in the editor's global settings panel and persisted in `editor_settings.yaml`.
  - Applied by a dedicated function (not the reflection system) to the active global settings: TAA on/off,
    AO quality/resolution, bloom, shadow resolution and cascade count, SSR.
  - Game mode gets **no** preset machinery — game developers set `GlobalSettings` from scripts; the engine
    only provides the underlying settings.
- Add reflected global settings: `ao_resolution` (half/quarter), `sm_resolution` (512/1024/2048/4096),
  `sm_cascade_count` (1–4). Renderer and shaders honor fewer cascades (shadow array layers, CSM split
  computation, cascade selection, particle shadow passes).
- Make LOW/MEDIUM presets actually save GPU time: TAA off must skip TAA + sharpen + velocity passes; bloom
  off must skip the bloom pass chain; both currently run unconditionally.
- Fix the two macOS problems: apply the window's vsync state to the GL context at startup; skip frame
  rendering (safely, no ImGui assert) while no display monitor is present, resuming automatically when the
  display wakes.
- **BREAKING (temporary tooling only)**: the ad-hoc env knobs `Z1_PROBE_SKIP_RENDER`, `Z1_PROBE_NO_AO`,
  `Z1_PROBE_AO_QUARTER`, `Z1_PROBE_NO_TAA`, `Z1_PROBE_NO_BLOOM`, `Z1_PROBE_NO_SHADOWS`,
  `Z1_PROBE_SHADOW_RES`, `Z1_PROBE_GAME_CAMERA` and the `util/frame_probe.h` header are removed; their
  permanent equivalents are the probing system and the new settings.

## Capabilities

### New Capabilities

- `performance-probing`: compile-time-gated CPU+GPU frame probing (scopes, reports, summary, GPU timer
  queries, diagnostic knobs) with zero overhead when disabled.
- `graphics-quality-presets`: editor-scoped LOW/MEDIUM/HIGH presets driving global render settings, with
  persistence and a dedicated apply function; explicitly unavailable in game builds.
- `shadow-cascade-configuration`: shadow map resolution and cascade count (1–4) as reflected settings,
  honored end-to-end by the renderers and shaders.
- `frame-loop-robustness`: vsync state applied for real at startup; render loop survives a powered-off
  display without crashing or wasting GPU.

### Modified Capabilities

- `screen-space-ambient-occlusion`: AO resolution becomes a setting (half or quarter of render resolution)
  instead of a fixed half-resolution requirement.

## Impact

- `premake5.lua`, `engine/runtime/premake5.lua` — new `ENABLE_PROBING` define (Hybrid), globs pick up new files
- `engine/runtime/source/util/prober.h/.cpp` — new probing module (replaces `util/frame_probe.h`)
- `engine/runtime/source/core/application.cpp` — probe integration; display-available frame guard
- `engine/runtime/source/core/window.h/.cpp` — `is_display_available()`; remove probe prints
- `engine/runtime/source/render/rhi/opengl_context.cpp` — apply vsync at startup; GPU query hooks; keep
  `Z1_NO_GL_CHECK` diagnostic
- `engine/runtime/source/render/global.h/.cpp` — new settings (`ao_resolution`, `sm_resolution`,
  `sm_cascade_count`), UBO mirror (`u_csm_cascade_count`), flush
- `engine/runtime/source/render/renderer/render_shared.h/.cpp` — configurable shadow FB (layers/res),
  N-cascade split computation, dynamic shadow resource recreation
- `engine/runtime/source/render/renderer/renderer_deferred.cpp`, `renderer_forward.cpp` — pass wiring
  (TAA/velocity/bloom skipping), cascade-count plumbing
- `engine/runtime/source/render/renderer/particle_renderer.cpp` — particle shadow passes use the configured
  cascade count
- `engine/content/shader/include/uniforms.glsl`, `include/lighting.glsl`, `particle.glsl` — cascade-count
  uniform and guarded cascade selection
- `engine/editor/source/quality_preset.h/.cpp` (new), `editor_layer.h/.cpp`, `editor_settings.yaml` —
  preset selection, persistence, apply function, global-panel UI
- `docs/PROFILING_MACOS_OPENGL.md` — update probing section to the formalized system
- `openspec/kb/` — knowledge-base notes on probing and quality settings

## Context

Both renderers draw offscreen through a `RenderGraph` whose intermediate framebuffers come from a
per-renderer `FramebufferPool` (keyed by pass name, reused when attachment specs match). Today every
attachment is single-sample:

- **Deferred** (`renderer_deferred.cpp`): `gbuffer` pass writes 5 MRT color attachments + depth
  (`gbuffer-position` RGBA32F, `-normal` RGB16F, `-albedo` RGBA8, `-metallic-roughness` RG16F,
  `-emissive` RGB16F, `-depth` Depth) and draws the skybox into the emissive attachment; `ao` reads
  depth/normal; `deferred-lighting` is a fullscreen pass sampling the G-buffer and writing `scene-color`
  (+ it depth-blits the G-buffer depth into `scene-depth`); `ssr` reads scene-color + G-buffer; the
  `forward-transparency` pass loads on top of the previous pass's framebuffer (passthrough); particles
  then TAA/velocity/bloom/post-process.
- **Forward** (`renderer_forward.cpp`): one `main` pass writes `scene-color` + `scene-depth` (opaque,
  blended, skybox); AO uses a separate depth+normal prepass.

Constraints that shape the design:

- Shaders are GLSL `#version 460` on Windows but `#version 410` on macOS; a shader that *statically* uses
  `gl_SampleID`/`gl_SampleMask` is executed at sample frequency for every non-discarded fragment, so bulk
  shading must live in a separate program that does not reference them.
- `sampler2D` cannot sample a multisampled texture; anything consuming the G-buffer or scene color with
  regular samplers (AO, SSR, velocity, TAA, bloom, post-process, materials) needs a resolved single-sample
  copy, which is exactly what a multisample→single-sample `glBlitFramebuffer` produces.
- The binding redesign gives every sampler a fixed slot stamped once; absent resources resolve to
  type-matched 1x1 fallbacks. A new sampler type (`sampler2DMS`) needs to join that machinery or the
  shader linker/reflection asserts.
- The Apple Intel GL driver is strict (a mismatched sampler type at draw is `GL_INVALID_OPERATION`) and
  re-specializes shaders when layouts change; MSAA must not perturb binding stability.
- Integrated GPUs pay for multisample memory: the deferred G-buffer is 5 MRTs, dominated by the RGBA32F
  position attachment (16 B/px/sample → 132 MB at 1080p × 4 samples).

## Goals / Non-Goals

**Goals:**

- A first-class `msaa_samples` setting (Off/2x/4x/8x) that anti-aliases geometry coverage in both
  pipelines, applies without restart, and degrades gracefully when the driver cannot honor it.
- Correct deferred MSAA: per-sample shading of edge pixels so silhouettes against the sky *and* against
  other geometry are anti-aliased, with the rest of the pipeline untouched (it consumes resolved,
  single-sample resources under their existing names).
- Zero change when the setting is Off: identical pass graphs, allocations and performance to today.
- MSAA and TAA compose (TAA remains MEDIUM/HIGH; MSAA is meaningful on its own, notably for LOW).

**Non-Goals:**

- Anti-aliasing blended transparency in the deferred pipeline (blended objects composite after the
  resolve; opaque and masked geometry get MSAA). Forward-pipeline transparency is inside the main pass
  and therefore *does* get MSAA.
- Alpha-to-coverage, MSAA for shadow maps, MSAA for AO/bloom/post targets (they are screen-space or
  separately filtered), and sample-count-dependent shader permutations beyond the edge pass.

## Decisions

### D1: Framebuffer-based MSAA, not window-buffer MSAA

`GLFW_SAMPLES` only affects the default framebuffer; the engine renders into FBOs and post-processes
before presenting, so the hint would do nothing for scene edges. MSAA is applied to internal render
targets. *Alternative rejected:* supersampling (scaling intermediate resolution) — 4× fill cost of every
pass including lighting, not MSAA, and this engine is fill-bound on integrated GPUs.

### D2: Multisampled textures, not renderbuffers

MSAA attachments are `GL_TEXTURE_2D_MULTISAMPLE` textures created with `glTexImage2DMultisample`
(GL 3.2; immutable storage, used instead of GL 4.3's `glTexStorage2DMultisample` because macOS caps at
GL 4.1). Renderbuffers would also work as render targets, but textures are
required for `texelFetch(sampler2DMS, pixel, sample)` per-sample attribute reads in the edge pass.
New RHI pieces: `Image2DMultiSample`, `ImageType::Image2DMultiSample`, `TextureTarget::Texture2DMultiSample`,
`Framebuffer::Attachment::samples`, multisample FBO attachment in `OpenGLFramebuffer::create`, sample-count
comparison in `FramebufferPool::is_reusable`, and a 1-sample fallback texture so unbound `sampler2DMS` slots
remain structurally valid (mirrors `render-resource-binding`'s typed fallbacks).

### D3: Deferred = multisampled G-buffer + resolve + sample-frequency edge pass

The G-buffer pass renders into multisampled attachments. A new `msaa-resolve-gbuffer` pass
(built from blits in `pre_pass`, hardware resolve) produces the canonical single-sample resources
(`gbuffer-position`, … `gbuffer-depth`); AO, SSR, velocity and bulk lighting keep consuming those names and
their code is untouched. The lighting pass writes `scene-color-ms` (multisampled RGBA32F, fullscreen quad →
identical color for all samples). A new **edge pass** then re-shades pixels whose per-sample depths differ
(edge = any sample's depth ≠ sample 0's), per sample:

- attributes from `texelFetch(u_*_ms, pixel, gl_SampleID)`,
- sky/background samples handled by the G-buffer rule (alpha == 0 → emissive),
- output masked with `gl_SampleMask[0] = 1 << gl_SampleID`.

A final `msaa-resolve-scene` blit produces the single-sample `scene-color` consumed by
SSR/TAA/bloom/post-process and by the deferred transparency pass.

*Alternatives rejected:* (a) resolving the G-buffer and lighting the resolved attributes only — edge
pixels then mix covered and uncovered samples (position pulled toward the clear value), producing bleeding
artifacts *worse* than no MSAA; (b) a post-process AA (FXAA-style) — not the requested feature and it
stacks poorly on TAA; (c) forward-only MSAA — the default pipeline is deferred, the feature would not
reach the editor's default view.

### D4: The edge pass is a shader *variant*, not a second shader file

`deferred_lighting.glsl` gains an `MSAA_EDGE` variant (new `ShaderVariant::MSAAEdge` bit, mapped to
`#define VARIANT_MSAA_EDGE 1`) and the `@variants:` block so `validate-shaders` compiles both combinations.
The shading body (attribute fetch → PBR/IBL/AO shadow lighting) is factored into a shared function that
both paths call; the bulk program does not reference `gl_SampleID` (stays per-pixel), the edge program
does (sample-frequency), and non-edge fragments `discard` after a cheap per-sample depth compare.
`u_gbuffer_*_ms` / `u_msaa_samples` declarations are inside `#ifdef VARIANT_MSAA_EDGE` so the bulk program
keeps its current sampler layout (Apple Intel re-specialization safety).

### D5: Forward = multisampled main pass + resolve

The forward `main` pass gains multisampled `scene-color-ms` + `scene-depth-ms` outputs and a resolve pass
produces `scene-color` + `scene-depth`. Nothing else changes; transparent geometry inside the pass is
anti-aliased natively. *Rationale:* the single-pass structure makes MSAA nearly free to add here, and it
keeps feature parity between render modes.

### D6: Canonical resource names stay single-sample; MSAA intermediates are suffixed

Consumers never observe the mode through names: with MSAA on, `gbuffer-*` and `scene-color`/`scene-depth`
are still single-sample resources (produced by resolve passes); the multisampled ones are named
`gbuffer-*-ms` / `scene-color-ms`. With MSAA off the extra passes are not added at all and names map
exactly as today. This keeps every downstream pass (AO, SSR, TAA chain, transparency, particles,
screenshot/picking paths) unmodified and makes the MSAA-off path bit-identical.

### D7: Setting, clamping and lifetime

`GlobalSettings::msaa_samples` is a reflected enum (`Off = 1, X2 = 2, X4 = 4, X8 = 8`) shown in the editor
inspector; it is *not* part of the Global UBO (only the edge pass needs the count, and it reads it from
`gl_NumSamples`). Renderers compute the effective count as
`min(requested, GL_MAX_SAMPLES, GL_MAX_COLOR_TEXTURE_SAMPLES, GL_MAX_DEPTH_TEXTURE_SAMPLES)` (all core since
GL 3.2; `glGetInternalformativ` is 4.2+ and unavailable on macOS);
if that yields < 2, MSAA is disabled for the frame and a `CORE_WARN` is logged once. Changing the setting
recreates the intermediate FBOs automatically through the pool's attachment comparison (`samples` becomes
part of `is_reusable`). Quality presets: LOW = 2x (no TAA there today), MEDIUM = 2x, HIGH = 4x — values
validated against frame-time probing on the reference Intel iGPU during implementation.

### D8: Depth resolves with a depth-writing draw, not a blit

`glBlitFramebuffer` only resolves **color** between different sample counts; depth/stencil blits require
matching sample counts, so multisample→single-sample depth blits are `GL_INVALID_OPERATION` (confirmed on
the macOS Intel driver). MSAA depth resolves therefore run as a small fullscreen pass
(`msaa_depth_resolve.glsl`) that reads `texelFetch(sampler2DMS, pixel, s)`, keeps the nearest (minimum)
sample and writes `gl_FragDepth`; the resolve passes clear their depth attachment to 1.0 first so the
depth test accepts every resolved value. Color resolves still use `blit_attachment` (hardware average
resolve), and single-sample→single-sample depth blits stay blits.

## Risks / Trade-offs

- [5-MRT multisampled G-buffer memory/bandwidth on integrated GPUs (RGBA32F position ≈ 132 MB @1080p×4)]
  → samples are clamped by the driver maximum; presets stay conservative (2x/2x/4x); Off remains the
  default for scenes that do not opt in; probing measures the cost in LOW/MEDIUM/HIGH.
- [Edge-detection failing (all pixels classified edge) would make the edge pass cost sample-frequency
  shading everywhere] → the detection is a depth-compare loop with early exit; probing (draw call counts,
  frame time) and A/B screenshots verify; the mechanism degrades to "slower", never to "wrong image".
- [Apple Intel driver strictness] → MSAA textures are immutable (no sampler-state writes); fallbacks are
  type-matched (`sampler2DMS` ← 1-sample MS texture); GL error checking stays enabled in Debug/Hybrid.
- [Blended transparency in deferred has no MSAA coverage (documented limitation)] → accepted for v1;
  transparency still benefits from TAA where enabled; forward mode gives it MSAA.
- [MSAA + TAA double-jitter interaction] → TAA jitters the projection; MSAA resolves geometry coverage
  beforehand; verify static + rotating screenshots for shimmer/ghosting regressions.
- [Framebuffer-pool churn when toggling the setting] → recreations happen only when `samples` actually
  changes; steady-state frames reuse cached FBOs.

## Migration Plan

Single change, no data migration: scenes without `msaa_samples` deserialize to the enum default (Off), so
existing content renders exactly as before. Rollback is setting the option back to Off (runtime) or
reverting the commit; both leave no persisted state behind.

## Open Questions

- Final preset sample counts (2x/4x vs 4x/4x) — decide from the probing numbers on the reference machine.
- Whether the edge pass should also run when `taa_enabled` is true (assumed yes: independent knobs).

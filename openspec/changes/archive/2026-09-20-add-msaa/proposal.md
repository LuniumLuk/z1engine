## Why

The engine's only anti-aliasing is TAA, and it is disabled in the LOW quality preset: geometric silhouettes
alias badly on low-end machines, and even with TAA on, thin geometry and freshly cut frames show hard edges
because temporal accumulation needs history. Hardware multisample anti-aliasing (MSAA) is the standard
complement — it anti-aliases rasterized geometry coverage directly, works without history, and is the only
AA path available when TAA is off.

## What Changes

- Add reflected global setting `msaa_samples` (Off / 2x / 4x / 8x), honored by both renderers and editable in
  the editor; applied at runtime (no restart). The effective sample count is clamped to the driver's
  `GL_MAX_SAMPLES` and per-format maximum; if MSAA is unavailable the setting degrades to Off with a
  `CORE_WARN`.
- RHI/framebuffer support for multisampled attachments:
  - new multisampled 2D image (`GL_TEXTURE_2D_MULTISAMPLE`, immutable storage) and `Texture2DMultiSample`
    sampler target, plus a typed 1-sample fallback for unbound `sampler2DMS` slots;
  - `samples` on `Framebuffer::Attachment`; the framebuffer pool recreates intermediate targets when the
    requested sample count changes.
- Deferred pipeline MSAA:
  - the G-buffer (5 MRT + depth) renders multisampled; a resolve pass produces the single-sample G-buffer
    (position/normal/albedo/metallic-roughness/emissive/depth) consumed by AO, SSR, velocity and bulk
    lighting — those stages are unchanged;
  - the lighting pass writes into a multisampled scene-color; a new sample-frequency **edge shading pass**
    (lighting shader variant) re-shades partially covered pixels per sample
    (`gl_SampleID`/`gl_SampleMask`), then a resolve produces the single-sample scene-color for
    SSR/TAA/bloom/post-process;
  - the sky (G-buffer emissive) gets per-sample edges against geometry for free because it is part of the
    multisampled G-buffer.
- Forward pipeline MSAA: the main pass (opaque + masked + blended + skybox) renders multisampled color+depth,
  followed by one resolve pass; downstream chain unchanged.
- Quality presets assign MSAA levels (LOW / MEDIUM / HIGH) in the editor preset function.
- With `msaa_samples = Off` the render graphs are exactly today's (no resolve/edge passes, no extra
  allocations, no behavior change).

Known limitations (documented, not bugs): blended transparency in the **deferred** pipeline composites after
the resolve, so blended objects have no MSAA coverage (opaque and masked geometry do); alpha-to-coverage is
out of scope; TAA and MSAA may be enabled simultaneously.

## Capabilities

### New Capabilities

- `multisample-anti-aliasing`: multisampled render targets for the G-buffer and forward passes, automatic
  resolve, per-sample edge shading in deferred lighting, and the `msaa_samples` setting with hardware
  clamping/fallback.

### Modified Capabilities

- `graphics-quality-presets`: presets SHALL additionally assign `msaa_samples` (per-preset values).
- `render-resource-binding`: typed fallback textures extend to multisample 2D samplers (`sampler2DMS`).

## Impact

- `engine/runtime/source/render/image.h`, `rhi/opengl_image.h/.cpp` — multisampled image type/target
- `engine/runtime/source/render/framebuffer.h`, `rhi/opengl_framebuffer.h/.cpp` — attachment sample counts,
  multisampled framebuffer creation
- `engine/runtime/source/render/render_graph.h/.cpp` — output sample counts, pool reuse comparison
- `engine/runtime/source/render/graphics_context.h`, `rhi/opengl_context.h/.cpp` — resolve blits, capability
  clamp, multisample fallback texture, target mapping
- `engine/runtime/source/render/data_types.h`, `shader.h`, `rhi/opengl_shader.cpp` — `Sampler2DMS`
  reflection/binding; `MSAA_EDGE` shader variant
- `engine/runtime/source/render/global.h` — `msaa_samples` setting + `REFLECTED_FIELD`
- `engine/runtime/source/render/renderer/renderer_deferred.h/.cpp` — MSAA graph (MSAA G-buffer, resolves,
  edge pass)
- `engine/runtime/source/render/renderer/renderer_forward.cpp` — MSAA main pass + resolve
- `engine/runtime/source/render/renderer/render_shared.h/.cpp` — shared resolve-pass helpers, effective
  sample-count helper
- `engine/content/shader/deferred_lighting.glsl` — `VARIANT_MSAA_EDGE` sample-frequency path, shared shading
  function
- `engine/editor/source/quality_preset.cpp` — preset sample counts
- `openspec/kb/render-pipeline.md`, `openspec/kb/perf-probing-and-quality.md` — design/limitation notes

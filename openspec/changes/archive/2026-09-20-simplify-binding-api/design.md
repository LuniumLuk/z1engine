# Design — Simplify Resource Binding API

## Context

Current state (as investigated 2026-09-18):

- `Image` (`render/image.h`, `render/image.cpp`) and `UniformBuffer` (`render/buffer.h`,
  `render/buffer.cpp`) own their binding: a global refcounted pool in `GraphicsContext` hands out texture
  units / UBO binding points on first `bind()` and takes them back on last `unbind()`. Call sites must pair
  `bind()`/`unbind()` manually and push the unit number into the shader via
  `set_uniform_binding(name, unit)`.
- Shader program state that is static in nature (sampler unit values, uniform block bindings) is re-written
  every pass and every draw (`opengl_shader.cpp`: `glUniform1i`, `glUniformBlockBinding`).
- Uniforms are resolved by name on every set: `std::unordered_map<std::string, uint32_t>` lookup + reflected
  type switch, even though full reflection data is already cached at link time.
- A macOS workaround (`m_default_sampler_binding`, highest unit holding 1x1 white 2D + 2D-array textures)
  stamps every sampler at link so programs that never set one still point at a target-correct texture; every
  consumer that can have an absent texture re-applies this fallback by hand (8+ call sites).
- Incident (2026-09-18): per-frame texture-unit layout churn made the Apple Intel GL driver JIT-compile
  shaders in ~1s storms whenever the visible draw set changed (fps 73→26 LOW, 91→45 HIGH). Interim fix:
  lowest-free-first allocation in `graphics_context.h`. The API still permits content-dependent layouts.

Constraints:

- OpenGL 4.1 core on macOS (no bindless `glBindTextureUnit`, no SSBOs, no `glClipControl`), Windows x64 is
  the primary platform; the macOS port must stay healthy.
- Shaders are preprocessed (`@uniforms`, `@reflections`, `@stage` sections); reflection is parsed at link in
  `OpenGLShader::link_shaders` — the raw material for slot tables already exists.
- Renderers: deferred + forward + particles + 2D + editor picking/preview all consume the same API.
- Verification on macOS is limited (test executables do not link there); probes (`Z1_PROBE_*`, `Z1_AUTOROTATE`)
  and smoke runs are the practical gates.

## Goals / Non-Goals

**Goals:**

- One obvious way to bind textures and uniform blocks; call sites declare *what*, the engine resolves *where*.
- Program binding state (sampler units, block bindings) is written once at link and never per frame.
- Deterministic, visible-content-independent binding layouts per program.
- No global binding state on resources (no pools, no refcounts, no `INVALID_BINDING` paths).
- Hot paths avoid string hashing; redundant GL calls are deduplicated.
- Rendering output identical; migrate all consumers (renderers, particles, 2D, editor picking/preview).
- Keep the model portable toward a future descriptor-set style RHI (Metal/Vulkan direction).

**Non-Goals:**

- Bindless texturing / descriptor indexing (not available on GL 4.1 / macOS).
- Full descriptor-set abstraction or RHI backend split (follow-up if/when the backend moves off GL).
- Vertex attribute / VAO layout redesign, sampler objects, buffer upload strategies.
- Shader authoring changes (no required edits to `.glsl` sources).

## Decisions

### D1. Fixed per-program sampler slots (slot == texture unit)

At link time, enumerate the program's sampler uniforms, sort them deterministically by name, assign
`slot k → GL texture unit k`, and stamp all sampler uniforms with `glUniform1i` exactly once. Slot tables are
cached on the shader (`name → { slot, unit, target type, location }`).

- Why: any per-frame change to sampler values is what made the macOS driver re-specialize shaders; removing
  the writes removes the failure class by construction and drops per-frame GL calls.
- Alternatives considered: (a) keep dynamic units but deduplicate — cheaper, but keeps per-frame
  `glUniform1i` and content-dependent layouts, so the incident can recur; (b) descriptor sets — the right
  long-term abstraction but a much larger refactor; the slot model is its stepping stone; (c) bindless — not
  available on the macOS GL 4.1 target.
- Consequence: `Image` no longer has binding state at all; texture units become an implementation detail of
  shader slots.

### D2. Deterministic slot ordering (sort by name, not reflection order)

Slots are assigned by ascending sampler name, not driver-reported index order. Re-linking the same sampler
set yields the same slot mapping everywhere.

- Why: predictable mapping, stable across drivers/variants, easier debugging and layout-hash probes.

### D3. Binder API on `GraphicsContext` with typed fallbacks and deduplication

New API surface (illustrative):

```cpp
struct TextureSlot { uint32_t m_unit; uint32_t m_index; };   // resolved once per shader

TextureSlot shader->sampler_slot(std::string const& name);   // cached
void ctx.bind_texture(TextureSlot slot, Image const* image, Image const& fallback);
void ctx.bind_uniform_block(UniformBlockId block, UniformBuffer const& buffer);
```

- The binder owns an invariant: at draw time every slot's unit holds a texture whose target matches the
  sampler type (real resource or type-correct 1x1 fallback: 2D, 2D-array, cube). This retires
  `m_default_sampler_binding` and all call-site fallback branches.
- Caches: `unit → GL handle` (skip redundant `glActiveTexture`/`glBindTexture`), `active unit`,
  `binding → GL handle` for blocks, so repeated binds of the same resources are free.
- Alternatives considered: keep `Image::bind(shader, name)` helpers — rejected; they re-introduce two
  sources of truth and manual refcounts.

### D4. Fixed UBO binding points by semantic

Reserve binding points by block name in one engine-side table (`Global=0`, `Lights=1`, `Bones=2`,
`PrevBones=3`, ...), apply `glUniformBlockBinding` once at link, and per frame only issue
`glBindBufferBase(binding, handle)` when the buffer for a binding changes (deduplicated).

- Why: block bindings are program state and were re-set per pass/draw for no reason; binding points become
  global constants shared by all programs.
- `UniformBuffer` loses `bind()/unbind()/get_binding()`; it keeps its GL handle and `write()`. Binding is a
  context operation keyed by the semantic binding id.
- Alternative: keep per-buffer dynamic binding points — rejected; it is the same churn machine.

### D5. Uniform handles and material binding plans

- `UniformHandle` (index into the shader's reflected uniform table) resolved once; `shader->set(handle, data)`
  performs no string lookup. Name-based `set_uniform(name, ...)` stays for cold paths; location-based setters
  are removed.
- `MaterialInstance` builds a binding plan per variant once (and on override change): a list of
  `(UniformHandle, value)` for scalars/vectors and `(TextureSlot, image | fallback)` for textures. `bind()`
  executes the plan without hashing or branching; texture binds go through the binder.

### D6. Render graph input binding through the binder

Passes declare graph inputs by name (already the case) plus their sampler slot mapping, e.g.
`node.bind_input("u_gbuffer_position", "gbuffer-position")`. Inputs are bound in one call at pass start;
optional inputs auto-fall back. `bind_input_index` + `set_uniform_binding` pairs are removed from renderer code.

### D7. Deletions (breaking, engine-internal)

- `Image`: `bind()`, `unbind()`, `bind(shader, name)`, `get_binding()`, `is_bound()`, `m_binding`,
  `m_ref_count`.
- `UniformBuffer`: `bind()`, `unbind()`, `bind(shader, name)`, `get_binding()`, `is_bound()`, `m_binding`,
  `m_ref_count`.
- `GraphicsContext`: `m_free_image_bindings`, `m_free_uniform_buffer_bindings`, both acquire/release pairs,
  `m_default_sampler_binding`, `create_default_sampler_textures` (replaced by typed fallbacks), dynamic
  binding counters.
- `Shader`: location-based binding setters and `get_uniform_binding` (superseded by slots/handles).

## Risks / Trade-offs

- [Per-program sampler count exceeds the fragment unit limit (16 on Intel macOS)] → count is ~10 today;
  assert at link with a clear error naming the program and sampler list; the sort-by-name slot assignment
  surfaces the count in the slot table for diagnostics.
- [Mixed vertex/fragment samplers would collide on the same unit namespace] → none exist today; reserve a
  high unit range for vertex-stage samplers if it ever appears (documented rule in the slot table).
- [Behavior differs subtly after migration (missing texture, wrong fallback type)] → the binder invariant
  makes mismatches structurally impossible; visual parity gates (screenshots) + probe A/B per phase.
- [Large diff touching renderers, materials, particles, editor] → phased migration (below); each phase is an
  independent commit and compiles/smokes on its own; old APIs remain until phase 4.
- [Slot tables per shader variant grow stale after hot reload] → slot table is rebuilt as part of link;
  hot-reload path recreates the shader object.
- [Rollback complexity] → no data or asset migration; reverting phase commits restores previous behavior.
  The min-heap allocator fix (2026-09-18) remains in place as an interim safety net until phase 1 lands.

## Migration Plan

- **Phase 0 — Metrics baseline:** add probe counters (texture binds/frame, sampler-uniform writes/frame,
  block-binding writes/frame, binding-layout hash) and record idle + `Z1_AUTOROTATE=2` baselines on the demo
  scene (probing build; macOS Intel reference machine).
- **Phase 1 — RHI foundation:** slot tables + `sampler_slot`, link-time stamping, typed fallback textures,
  uniform handles, binder with dedup caches, fixed UBO bindings. Old APIs still work (dual support); no
  visual change.
- **Phase 2 — Pass conversion:** convert `render_shared`, `renderer_deferred`, `renderer_forward`,
  `particle_renderer`, bloom/post chain, picking, 2D renderer to the binder; delete manual fallback branches.
- **Phase 3 — Materials:** binding plans in `MaterialInstance`; remove per-draw `Image::bind(shader, name)`
  usage; mesh draw paths stop pairing bind/unbind.
- **Phase 4 — Cleanup:** delete pools, refcounts, default-sampler machinery, deprecated shader setters; KB
  and `render-pipeline.md` updates.
- Gates per phase: `python dev/z1.py compile` (0 errors), `smoke`, probe A/B vs Phase 0 metrics, screenshot
  parity for the demo scene; final `python dev/z1.py dcv --auto` per `change-validation-gates`
  (correctness + benchmark verdict). On macOS the pre-existing test-link gap is waived explicitly and
  compile + smoke + probe gates apply.

## Open Questions

- Input→uniform name mapping in the graph: explicit `node.bind_input("u_x", "input-name")` (recommended) vs
  auto-derivation (`u_` + name with dashes→underscores). Decide in phase 2.
- How to expose the "sampler writes per frame" counter for tests (probe-only vs always-on lightweight stats).
- Whether to adopt `glProgramUniform*` (GL 4.1) to avoid binding the program for uniform updates — optional
  micro-optimization, decide after phase 1 measurements.
- Whether material binding plans should live on `Material` (shared) or `MaterialInstance` (per-instance
  overrides) long-term; start on `MaterialInstance`, measure, revisit.

## Baseline Metrics (Phase 0, 2026-09-18)

Demo scene, LOW preset, 640x480 viewport, probing Hybrid, macOS Intel Iris Plus 645 (301-frame runs, counters
are per-frame averages; `renderer_draw` averages include the startup compile spike, steady state ~0.5 ms):

| Counter | Idle | Rotating (`Z1_AUTOROTATE=2`) |
|---|---|---|
| draws | 11.00 | 9.55 |
| tex_binds | 34.00 | 27.44 |
| tex_unbinds | 34.00 | 27.44 |
| sampler_writes | 25.00 | 19.21 |
| block_writes | 10.00 | 8.55 |
| name_resolutions | 117.00 | 86.58 |
| ubo_binds | 2.00 | 2.00 |
| renderer_draw (ms) | 2.86 avg | 2.82 avg |

These are the pre-change reference points; post-change gates expect sampler/block writes and name
resolutions to converge on 0 per frame in steady state and texture binds to drop with deduplication.

## Post-Implementation Metrics (Phases 1-3 complete)

Same harness (probing Hybrid, macOS Intel Iris Plus 645, idle 301-frame runs unless noted). Phases 1-2 were
behavior-neutral and verified incrementally (P2a: tex_binds 29, sampler_writes 19, block_writes 7,
name_resolutions 108; P2b: 21 / 11 / 5 / 98; P2c: 19 / 11 / 4 / 97; P2d: 18 / 10 / 4 / 96); Phase 3 removed
the remaining material-path writes:

| Counter | Idle (Phase 3) | Notes |
|---|---|---|
| draws | 11.00 | unchanged |
| tex_binds / tex_unbinds | 0 per frame | full dedup; no per-frame legacy binds remain |
| sampler_writes | 0 per frame | sampler values stamped once at link |
| block_writes | 0 per frame | semantic UBO bindings resolved at link; binder dedups buffer binds |
| ubo_binds / ubo_unbinds | 1.00 | single legacy `Global` path remains (deleted in §5) |
| name_resolutions | 25.00 (steady) | pass uniforms: `u_csm_index`, `u_has_skinning`, `u_prev_model`, ... |
| layout_hash | 4042235446 (constant) | per-program layout stable across 602 frames |
| renderer_draw (ms) | 0.44-0.50 steady | camera-motion (`Z1_AUTOROTATE=2`, 600 frames) max 1.58 ms after warmup |

Camera-rotation regression (the original P0 symptom) stays smooth: no 50 ms+ `renderer_draw` spikes across
600 rotating frames, 0 OpenGL errors in all runs.

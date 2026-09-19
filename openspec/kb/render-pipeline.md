# Render Pipeline
> Summary: Render graph architecture, deferred/forward pipelines, and RHI abstraction
> Scope: engine/runtime/source/render/, engine/runtime/source/render/renderer/, engine/runtime/source/render/rhi/

## Render Graph

- `RenderGraph` -- manages render pass execution order via dependency graph
- `RenderGraphNode` -- individual render pass with inputs/outputs
- Nodes declare resource dependencies; graph resolves execution order
- Key file: `render/render_graph.h`

## Pipeline Architecture

### Deferred Pipeline (`renderer/renderer_deferred.h`)

| Pass | Shader | Output |
|------|--------|--------|
| G-Buffer | `gbuffer.glsl` | Albedo, normal, position, material buffers |
| Shadow | `shadow.glsl` | Shadow map |
| AO (SSAO/GTAO) | `ssao.glsl` / `gtao.glsl` | Half- or quarter-res AO buffer (optional `ao_blur.glsl`) |
| Lighting | `deferred_lighting.glsl` | Lit color buffer |
| Skybox | `deferred_skybox.glsl` | Background fill |

### Ambient Occlusion (2026-08-14)

Both pipelines support screen-space AO (SSAO or Jimenez GTAO), controlled by `GlobalSettings` AO fields (`ao_enabled`, `ao_type`, `ao_resolution`, `ao_radius`, `ao_intensity`, `ao_power`, `ao_bias`, `ao_blur_enabled`, `ao_blur_strength`) exposed in the editor inspector under `ambient_occlusion`.

- AO computed at **half or quarter resolution** (RGBA8, selected by `ao_resolution`) in view space from depth + world-space normal; position reconstructed from depth via inverse projection. The buffers are sized in `RenderShared::ensure_buffers`; changing the setting recreates them via the existing size check.
- `RenderShared::add_ao_pass(rg, depth_input, normal_input)` adds `"ao"` (+ `"ao-blur"` when enabled) and returns the final pass name; consumers call `depends_on()` on it and bind `get_ao_image()`.
- **Deferred**: AO pass reads `gbuffer-depth`/`gbuffer-normal` after the G-buffer pass; `deferred_lighting.glsl` multiplies ambient by AO.
- **Forward**: a `prepass` (depth+normal) renders opaque+mask geometry with the GBuffer shader variant before the AO pass; forward PBR/phone shaders sample `u_ao_texture` at `v_screen_uv` (location 8 varying) and multiply ambient.
- Materials receive the AO texture through `PerFrameConst::ao_map` and bind it to the `u_ao_texture` sampler slot; shaders gate sampling on the global `u_ao_enabled` flag.
- Verified algorithm references: `D:\wiki\wiki\concepts\z1engine-gtao-ssao.md` (GTAO per-slice integral `0.25·(cosN + 2h·sin n − cos(2h−n))`, horizon clamping, slice weights).

### Forward Pipeline (`renderer/renderer_forward.h`)

- Single-pass rendering for transparent/special objects
- Uses `pbr.glsl`, `unlit.glsl`, `sprite_2d.glsl`
- Default material (`MI_phone` / `phone.glsl`) uses Cook-Torrance PBR lighting (same as deferred) for brightness parity

### Particle Renderer (`renderer/particle_renderer.h/.cpp`)

- Renders GPU particles as camera-facing billboards via instanced draw
- `particle.glsl` -- vertex + fragment shader for particle quads
- `particle_shadow.glsl` -- depth-only shadow pass shader using `u_sun_projview[u_csm_index]`
- Instance data: position, color, size, rotation per alive particle
- VBO dynamically resized to match `m_max_particles * sizeof(ParticleInstanceData)`
- `add_particle_pass(rg, scene, input_pass, shadow_image)` -- shadow_image enables CSM shadow reception per emitter
- `add_particle_shadow_passes(rg, scene, shadow_fb, csm_layers)` -- appends one depth-only pass per configured cascade after mesh shadow passes using `LoadOp::Load`
- Shadow receive: binds CSM shadow array to the `u_shadow_map` sampler slot, binds the `Global` block via `GraphicsContext::bind_uniform_buffer`, sets `u_receive_shadows` per emitter
- Shadow cast: per-cascade billboard depth pass; skips emitters with `m_cast_shadows = false`
- Both controlled by `ParticleComponent::m_receive_shadows` and `m_cast_shadows` (default `true`)

### Post-Processing

| Pass | Shader |
|------|--------|
| Bloom downsample | `bloom_downsample.glsl` |
| Bloom upsample | `bloom_upsample.glsl` |
| TAA | `taa.glsl` |
| TAA Sharpen | `taa_sharpen.glsl` |
| Velocity | `velocity.glsl` |
| SSAO | `ssao.glsl` |
| GTAO | `gtao.glsl` |
| AO blur | `ao_blur.glsl` |
| SSR | `ssr.glsl` |
| Tone mapping | `postprocessing.glsl` |

### Shared Infrastructure (`renderer/render_shared.h`)

- Common render state and utilities shared between deferred and forward
- `Renderer` -- high-level interface: `Renderer::submit()`
- `Renderer2D` -- 2D sprite batching (`renderer/renderer_2d.h`)

## Shadows (cascaded shadow maps)

- `GlobalSettings::sm_resolution` (512/1024/2048/4096) and `sm_cascade_count` (1-4) configure the CSM array;
  both are reflected/serialized and editable under the `shadow` group.
- `RenderShared::ensure_shadow_resources()` (called from `ensure_buffers()`) allocates the depth array with
  exactly `sm_cascade_count` layers at `sm_resolution`² and recreates it only when either setting changes.
- `calculate_csm_splits` computes N split distances (lambda 0.95) and N light matrices; unused
  `sun_projview` slots repeat the last valid cascade so shaders never read uninitialized matrices.
- Light-frustum sizing (2026-09-18 fix): the nearest cascade's ortho half-extent is `sm_ortho_size`;
  farther cascades scale as `sm_ortho_size * (split_far / split_1)`, clamped to the camera far plane.
  Deriving the extent from the camera far plane (`diag/2 + cascade_far`) shrank the whole scene into a
  couple of shadow-map texels whenever the far plane was large (e.g. 1000) — one-cascade setups then had
  no visible shadows at all. `sm_ortho_size` was previously an unused reflected knob; it now controls
  shadow coverage vs. sharpness. The light-view texel snap writes translation in column 3
  (`light_view[3][0..1]`) — indexing `[0][3]` snapped the zero bottom row and was a silent no-op.
- Shaders pick the cascade with `u_csm_cascade_count` guards (`include/lighting.glsl::get_cascade_index`,
  `particle.glsl`): only existing layers are sampled, beyond-range distances clamp to the last cascade.

### TAA Pipeline (2026-07-23 upgrade)

TAA now uses a modern algorithm with: jitter compensation (UV offset passed to
resolve shader), per-pixel adaptive blend from 5x5 neighborhood YCoCg variance,
UE4-style variance-guided AABB clip, and a separate luma-guided unsharp mask
sharpen pass (`taa_sharpen.glsl`) inserted between TAA resolve and bloom.

- Jitter: Halton(2,3) sequence applied to projection matrix; offset stored in
  `GlobalSettings::taa_jitter_uv` and passed to `taa.glsl` as
  `u_taa_jitter_u`/`u_taa_jitter_v`
- New tunables in global UBO: `taa_variance_scale`, `taa_clip_gamma`,
  `taa_sharpen_enabled`, `taa_sharpen_strength`
- `taa_blend` semantics changed from 0.9 (history weight) to 0.1 (new-frame weight)
- Sharpen pass reuses `history_colors[read_idx]` as output target (overwritten
  after serving as TAA history input)
- **Conditional passes:** with `taa_enabled == false` the renderers do not add the velocity,
  TAA or sharpen passes at all and the post-process pass reads the scene color (or SSR output) directly;
  with `pp_bloom_enabled == false` the bloom chain is skipped entirely and post-process samples no bloom texture.
  `add_bloom_pass(rg, input)` and `add_postprocess_pass(rg, target, scene_input, bloom_present)` take the
  chain inputs as parameters.

### Screen-space reflections (2026-09-19 fix)

- Deferred-only: `RendererDeferred::add_ssr_pass` ray-marches world space against `gbuffer-position`
  (thickness-based hit test with 5-step binary refine), samples `scene-color` at the hit, writes
  `scene-color-ssr`; TAA/post-process consume that output. Enabled by `GlobalSettings::ssr_enabled` +
  the scene's `ssr_*` values (HIGH preset turns it on, LOW forces it off).
- Reflection weight is physical now: `F0 = mix(0.04, base_color, metallic)` with Schlick Fresnel, times the
  hit confidence (`hit_weight`) and `u_ssr_intensity`; the demo scene authors intensity 2.0.
- The pre-fix chain multiplied `mix(0.04, 1.0, metallic)` by `(0.2 + 0.8 * fresnel)` *and*
  `(1.0 - roughness)` *and* a `1.0 - miss/thickness` stability term — on a metallic floor that collapsed the
  reflection to ≲2% of the sampled color, so SSR was invisible at every preset (A/B screenshots: max delta
  22/255; now 119/255 with a visible mirror ghost). The march uses the jittered `u_projview` plus per-pixel
  temporal jitter, so reflections rely on TAA (enabled at MEDIUM/HIGH) to resolve cleanly.

## RHI (Render Hardware Interface)

- OpenGL backend in `render/rhi/`
- Abstractions: `VertexBuffer`, `Shader`, `Texture`, `Framebuffer`, `Pipeline`

| Abstraction | Header | OpenGL impl |
|-------------|--------|-------------|
| Buffer | `buffer.h` | `rhi/opengl_buffer.h` |
| Context | `graphics_context.h` | `rhi/opengl_context.h` |
| Framebuffer | `framebuffer.h` | `rhi/opengl_framebuffer.h` |
| Image | `image.h` | `rhi/opengl_image.h` |
| Pipeline | `pipeline.h` | `rhi/opengl_pipeline.h` |
| Shader | `shader.h` | `rhi/opengl_shader.h` |
| Vertex Array | `vertex_array.h` | `rhi/opengl_vertex_array.h` |

### Sampler bindings (2026-09-18 redesign; archived 2026-09-19)

See `openspec/changes/archive/2026-09-19-simplify-binding-api/`. Phases 1-3 are implemented and verified;
Phase 4 cleanup and the final regression tasks were not executed, so the legacy API remains in dual-support
mode:

- Each program gets **fixed sampler slots** at link: samplers sorted by name, `slot == unit`, values stamped
  once via `glProgramUniform1i/1iv`. `Shader::sampler_slot(name)` returns `{ slot, unit, location, type }`.
- Textures bind through `Shader::bind_texture(slot, image)` (or `(slot, element, image)` for sampler arrays);
  a null image resolves to a **type-matched 1x1 white fallback** (2D / 2D-array / cube), so absent optional
  resources are structurally safe — no call-site fallback branches.
- Uniform blocks use **semantic bindings** (`render/uniform_blocks.h`: Global=0, Lights=1, Bones=2,
  PrevBones=3) applied once at link; buffers bind via `GraphicsContext::bind_uniform_buffer(binding, buffer)`
  (dedup-cached). A program declaring a block outside the table logs a `CORE_ERROR` at link (it would
  otherwise keep the driver default binding 0 and silently alias `Global`); add new blocks to
  `render/uniform_blocks.h`.
- `GraphicsContext::bind_texture_unit(unit, handle, target)` dedups unit→handle pairs; renders submit
  bound state instead of acquire/release-ing pooled units per draw.
- Value uniforms use `Shader::uniform_handle(name)` + `set_uniform(handle, data)` on hot paths;
  `set_uniform(name, ...)` remains for cold/pass-specific uniforms.
- Materials execute a cached per-shader **binding plan** (handles + slots resolved once, rebuilt when the
  shader variant or overrides change); `PerFrameConst` carries resource pointers (`lights`, `shadow_map`,
  `ao_map`, `sky_ibl_map`) instead of binding indices.
- The legacy `m_default_sampler_binding` machinery (highest unit holding white 2D/2D-array textures) is
  superseded by the typed fallbacks; removing it (with the binding pools) is the unexecuted Phase 4 cleanup
  of the archived change.

Why typed fallbacks matter: a sampler left at GL's default value (unit 0) can reference a texture of a
mismatched target (e.g. `sampler2D` on the CSM `GL_TEXTURE_2D_ARRAY`), which macOS drivers reject with
`GL_INVALID_OPERATION` at draw time; Windows drivers silently tolerate it.

### Binding allocation stability (2026-09-18)

- Image/UBO bindings are handed out **lowest-free-first** (`m_free_image_bindings` /
  `m_free_uniform_buffer_bindings` are min-heaps in `graphics_context.h`).
- Do not regress to history-dependent allocation (the old LIFO `std::stack` made the unit
  layout rotate every frame): the Apple Intel GL driver re-specializes shaders per sampler
  layout and JIT-compiles them at draw time, which showed up as 100ms+ frame spikes and
  fps drops while the camera moved (details in `perf-probing-and-quality.md`).
- Layout must depend only on what is bound at a point in the frame, never on visible
  content or previous-frame release order.

## Key Types

- `ImageFormat` -- pixel format enum (in `data_types.h`)
- `RenderPass` -- render pass config (`render_pass.h`)
- `Resource` -- GPU resource base (`resource.h`)

## Multi-Scene Rendering (2026-08-20, material editor)

- `RendererDeferred::draw(scene, fb)` / `RendererForward::draw(scene, fb)` take the scene explicitly (`scene->get_main_camera()`); they never touch `g_runtime_context.m_scene`. A new in-memory `Scene` needs only a main camera + lights + mesh entities.
- `RenderShared` (TAA history, `m_frame_index`, lights UBO, AO/bloom buffers) is per-renderer-instance. Rendering two scenes per frame with ONE renderer instance cross-contaminates TAA history — create a second `RendererDeferred`/`RendererForward` instance for the second scene.
- **Per-scene intermediate framebuffers**: `RenderGraph` no longer owns a process-wide static framebuffer cache. Each `RenderShared` (one per renderer instance) owns a `FramebufferPool` (`m_framebuffer_pool`) and passes it to the per-frame graph via `RenderGraph::set_framebuffer_pool()`. Intermediate passes (`gbuffer`, `scene-color`, `velocity`, `taa-sharpen`, ...) are cached per pool/pass-name; a second scene rendered through its own renderer gets its own intermediates and never resizes or reuses the first scene's. `FramebufferPool::is_reusable` compares attachment count + format/sampler/wrap/layers; size mismatch on reuse calls `resize()` (only happens on real target-size changes, e.g. window resize). Graphs without an attached pool (e.g. `Renderer2D`) fall back to a per-graph internal pool.
- Shared `GlobalSettings` (UBO) is a scratch slot mutated per draw. `prev_projview` is tracked per renderer instance (`RenderShared::m_prev_projview`): each `draw()` loads its own previous-frame matrix into the UBO before the velocity/TAA passes and stores the new one afterwards, so scenes are independent of draw order. `RenderShared::update_sky_light` re-pushes cached SH coefficients to `g->sky_sh` on every draw (a sky-less secondary scene zeroes them, which must not stick).
- Draw order per frame in `EditorLayer::on_update`: preview scene render → `scene->on_update(0)` (prev transforms) → main scene draw.

-> see [shader-system.md]
-> see [architecture.md]

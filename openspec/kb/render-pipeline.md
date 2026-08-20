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
| AO (SSAO/GTAO) | `ssao.glsl` / `gtao.glsl` | Half-res AO buffer (optional `ao_blur.glsl`) |
| Lighting | `deferred_lighting.glsl` | Lit color buffer |
| Skybox | `deferred_skybox.glsl` | Background fill |

### Ambient Occlusion (2026-08-14)

Both pipelines support screen-space AO (SSAO or Jimenez GTAO), controlled by `GlobalSettings` AO fields (`ao_enabled`, `ao_type`, `ao_radius`, `ao_intensity`, `ao_power`, `ao_bias`, `ao_blur_enabled`, `ao_blur_strength`) exposed in the editor inspector under `ambient_occlusion`.

- AO computed at **half resolution** (RGBA8) in view space from depth + world-space normal; position reconstructed from depth via inverse projection.
- `RenderShared::add_ao_pass(rg, depth_input, normal_input)` adds `"ao"` (+ `"ao-blur"` when enabled) and returns the final pass name; consumers call `depends_on()` on it and bind `get_ao_image()`.
- **Deferred**: AO pass reads `gbuffer-depth`/`gbuffer-normal` after the G-buffer pass; `deferred_lighting.glsl` multiplies ambient by AO.
- **Forward**: a `prepass` (depth+normal) renders opaque+mask geometry with the GBuffer shader variant before the AO pass; forward PBR/phone shaders sample `u_ao_texture` at `v_screen_uv` (location 8 varying) and multiply ambient.
- Materials receive the AO texture via `PerFrameConst::ao_map_binding`; shaders gate sampling on the global `u_ao_enabled` flag.
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
- `add_particle_shadow_passes(rg, scene, shadow_fb, csm_layers)` -- appends 4 depth-only passes after mesh shadow passes using `LoadOp::Load`
- Shadow receive: binds CSM shadow array, calls `set_uniform_block_binding("Global", ...)`, sets `u_receive_shadows` per emitter
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
| Tone mapping | `postprocessing.glsl` |

### Shared Infrastructure (`renderer/render_shared.h`)

- Common render state and utilities shared between deferred and forward
- `Renderer` -- high-level interface: `Renderer::submit()`
- `Renderer2D` -- 2D sprite batching (`renderer/renderer_2d.h`)

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

## Key Types

- `ImageFormat` -- pixel format enum (in `data_types.h`)
- `RenderPass` -- render pass config (`render_pass.h`)
- `Resource` -- GPU resource base (`resource.h`)

## Multi-Scene Rendering (2026-08-20, material editor)

- `RendererDeferred::draw(scene, fb)` / `RendererForward::draw(scene, fb)` take the scene explicitly (`scene->get_main_camera()`); they never touch `g_runtime_context.m_scene`. A new in-memory `Scene` needs only a main camera + lights + mesh entities.
- `RenderShared` (TAA history, `m_frame_index`, lights UBO, AO/bloom buffers) is per-renderer-instance. Rendering two scenes per frame with ONE renderer instance cross-contaminates TAA history — create a second `RendererDeferred`/`RendererForward` instance for the second scene.
- `RenderGraph::s_cached_framebuffers` is a static map keyed by pass name only; `cache_is_reusable` ignores size and `resize()`s on mismatch. Two targets with different resolutions using the same pass names reallocate intermediates every frame. The material editor uses a fixed 512x512 preview and accepts this churn (secondary draw runs first, then the main viewport resizes intermediates back).
- Shared `GlobalSettings` (UBO) is mutated per draw (`projview`, `cam_position`, `prev_projview`, sky SH). Draw the secondary scene FIRST so the main viewport draw leaves these values correct for the next frame (main velocity pass reads `prev_projview`).
- Draw order per frame in `EditorLayer::on_update`: preview scene render → `scene->on_update(0)` (prev transforms) → main scene draw.

-> see [shader-system.md]
-> see [architecture.md]

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
| Lighting | `deferred_lighting.glsl` | Lit color buffer |
| Skybox | `deferred_skybox.glsl` | Background fill |

### Forward Pipeline (`renderer/renderer_forward.h`)

- Single-pass rendering for transparent/special objects
- Uses `pbr.glsl`, `unlit.glsl`, `sprite_2d.glsl`

### Particle Renderer (`renderer/particle_renderer.h/.cpp`)

- Renders GPU particles as camera-facing billboards via instanced draw
- `particle.glsl` -- vertex + fragment shader for particle quads
- Instance data: position, color, size per alive particle
- VBO dynamically resized to match `m_max_particles * sizeof(ParticleInstanceData)`

### Post-Processing

| Pass | Shader |
|------|--------|
| Bloom downsample | `bloom_downsample.glsl` |
| Bloom upsample | `bloom_upsample.glsl` |
| TAA | `taa.glsl` |
| Velocity | `velocity.glsl` |
| Tone mapping | `postprocessing.glsl` |

### Shared Infrastructure (`renderer/render_shared.h`)

- Common render state and utilities shared between deferred and forward
- `Renderer` -- high-level interface: `Renderer::submit()`
- `Renderer2D` -- 2D sprite batching (`renderer/renderer_2d.h`)

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

-> see [shader-system.md]
-> see [architecture.md]

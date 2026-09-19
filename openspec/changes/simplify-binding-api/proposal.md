# Simplify Resource Binding API

## Why

Per-frame binding churn is both a correctness hazard and a measured performance problem: the engine
rebroadcasts static program state (sampler units, UBO block bindings) every pass and every draw, allocates
texture units dynamically through a global refcounted pool, and resolves uniforms by string in hot paths.
On macOS this layout instability made the Apple GL driver JIT-compile shaders in ~1s storms whenever the
camera moved (interim mitigation: min-heap allocator, 2026-09-18). The API also forces every call site to
pair `bind()`/`unbind()` manually and to hand-patch absent textures with the default-sampler workaround,
duplicating fallback logic across 8+ files.

## What Changes

- Give every shader program **fixed sampler slots** assigned once at link time (slot == texture unit,
  sampler uniforms stamped once); sampler values are never written per frame again.
- Replace `Image::bind()/unbind()/get_binding()` + `Shader::set_uniform_binding` pairs with a **binder API**
  on `GraphicsContext` (`bind_texture(slot, image, fallback)`, `bind_uniform_block(binding, buffer)`) that
  binds to the slot and deduplicates redundant GL calls.
- Provide **type-correct fallback textures** (2D / 2D-array / cube) so an absent resource can never leave a
  sampler pointing at a mismatched target; remove the `m_default_sampler_binding` workaround and its
  call-site fallbacks.
- **BREAKING** (engine-internal RHI): remove image/UBO global binding state (unit pools, refcounts,
  `INVALID_BINDING` handling) and the shader's location-based binding setters.
- Assign **fixed UBO binding points by semantics** (Global, Lights, Bones, ...) once at link; per frame only
  `glBindBufferBase` is issued (deduplicated).
- Add **uniform handles** (reflection-resolved indices) and **material binding plans** so pass/material
  bindings execute without string hashing; keep name-based `set_uniform` for cold paths.
- Render graph inputs bind through the binder (`node.bind_input("u_x", "gbuffer-x")`) instead of
  `bind_input_index` + `set_uniform_binding`.

## Capabilities

### New Capabilities

- `render-resource-binding`: shader/resource binding model — fixed sampler slots, binder API with typed
  fallbacks and deduplication, fixed UBO binding points, uniform handles, material binding plans, and the
  binding-layout stability guarantee.

### Modified Capabilities

- (none)

## Impact

- RHI/runtime: `render/image.{h,cpp}`, `render/buffer.{h,cpp}`, `render/shader.h`,
  `render/rhi/opengl_shader.{h,cpp}`, `render/rhi/opengl_image.{h,cpp}`, `render/rhi/opengl_buffer.{h,cpp}`,
  `render/graphics_context.{h,cpp}`
- Renderers/passes: `renderer_deferred`, `renderer_forward`, `render_shared`, `particle_renderer`,
  `render_graph.{h,cpp}`, `renderer_2d`
- Assets: `asset/material.{h,cpp}`, `asset/mesh.cpp`
- Editor: `picking_system`, material editor preview
- Behavior: rendering output identical; binding-related GL calls and per-frame program-state writes drop
  substantially; the macOS shader recompile-storm class is eliminated by construction

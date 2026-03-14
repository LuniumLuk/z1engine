## Context

The particle system was implemented in `2026-03-14-particle-system` as a single-commit feature. It is functional and renders correctly, but the implementation has a simulation bug (size_over_life), architectural coupling (config mixed with GPU state), performance inefficiencies (CPU quad expansion, linear dead-slot scan), and shader fragility (texture detection via texcoord comparison, camera vector matrix decomposition). The codebase follows established ECS patterns (AnimationSystem, Renderer2D, SpriteComponent) that the particle system partially deviates from.

All changes target files introduced in the original particle system commit. No existing pre-particle code is modified except `scene.cpp` (indentation fix and error handling in the particle deserialization block that was added in the same commit).

## Goals / Non-Goals

**Goals:**
- Fix the `size_over_life` simulation bug so particles interpolate size correctly over their lifetime.
- Separate emitter configuration (reflected, serialized) from runtime state (GPU buffers, particle pool) in `ParticleComponent`.
- Replace O(n) dead-slot scanning with O(1) free-list allocation for particle spawning.
- Deduplicate repeated code in pipeline creation and extract `ParticleVertex` to a shared header.
- Replace fragile shader patterns (texcoord-based texture detection, projview matrix decomposition) with explicit uniforms.
- Implement GPU instanced rendering to reduce per-frame vertex upload by 75%.
- Implement actual depth-based soft particle blending in the Soft pipeline.
- Add error handling to particle YAML deserialization.
- Fix indentation errors in `scene.cpp`.

**Non-Goals:**
- GPU compute shader simulation (future work for 100K+ particles).
- Texture atlas / sprite sheet animation.
- Color-over-life gradient curves (beyond the current 2-point lerp).
- Sub-emitters or particle trails.
- Velocity-aligned (stretched) billboards.
- Unit tests (separate change).

## Decisions

### 1. Store `birth_size` in Particle struct

**Decision**: Add a `float birth_size` field to the `Particle` struct. During simulation, compute display size as `birth_size * lerp(size_over_life.x, size_over_life.y, t)` and store the result in `p.size` for rendering. The `birth_size` is set once at spawn time and never modified.

**Alternative considered**: Keep a separate array of birth sizes indexed by particle slot. Rejected because it adds complexity and cache misses for no benefit -- the `Particle` struct already has padding room.

### 2. Runtime state separation via nested struct (not a separate component)

**Decision**: Group runtime-only fields (`m_particles`, `m_alive_count`, `m_emission_accumulator`, `m_total_emitted`, `m_vbo`, `m_vao`, `m_ibo`, `m_free_list`) into a `struct ParticleRuntimeState` defined inside `particle.h`. The `ParticleComponent` holds one instance: `ParticleRuntimeState m_runtime`. This keeps all particle data co-located in the ECS component (no additional registry lookup) while clearly separating serialized config from transient state.

**Alternative considered**: Move runtime state into a separate ECS component (`ParticleRuntimeComponent`). Rejected because it would require a second `view.get<>()` call in both the system and renderer hot paths, adding indirection for no architectural benefit.

### 3. Free-list via index stack

**Decision**: Add a `std::vector<uint32_t> m_free_list` to `ParticleRuntimeState`. On initialization, push all indices `[0, max_particles)` onto the stack. On spawn, pop the top index (O(1)). On particle death, push its index back (O(1)). This replaces the current O(n) linear scan in the spawn loop.

**Alternative considered**: Swap-with-last compaction (used by some engines). Rejected because it would invalidate depth sorting order and require re-indexing, adding complexity.

### 4. Instanced rendering with shared quad VBO

**Decision**: Create a single static quad VBO (4 vertices, 6 indices) shared across all particle emitters. Per-particle data (position, size, color, rotation) is uploaded to a separate instance VBO with `glVertexAttribDivisor(attr, 1)`. The draw call becomes `glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, alive_count)`. This reduces per-frame upload from `alive_count * 4 * sizeof(ParticleVertex)` to `alive_count * sizeof(ParticleInstanceData)`.

**Alternative considered**: Keep the current 4-vertices-per-particle approach with `gl_VertexID % 4` expansion. Rejected because it wastes 75% of upload bandwidth duplicating position/color/size across 4 vertices.

### 5. Explicit camera uniforms

**Decision**: Pass `u_cam_right` (vec3) and `u_cam_up` (vec3) as explicit shader uniforms set from `CameraComponent` in the renderer. Remove the projview matrix decomposition from the vertex shader.

**Rationale**: Extracting camera axes from the projview matrix is incorrect when the projection has non-uniform scaling (e.g., different aspect ratios). Explicit uniforms are cheaper and correct.

### 6. `u_has_texture` integer uniform

**Decision**: Add `uniform int u_has_texture;` to the particle shader. The renderer sets it to 1 when a texture is bound, 0 otherwise. The fragment shader uses `u_has_texture > 0` to decide whether to sample the texture or use white.

**Rationale**: The current `tex_color.a < 0.001 && v_texcoord == vec2(0.0)` check is unreliable -- it fails for particles at texcoord (0,0) with a bound texture, and for unbound textures where the sampler returns non-zero garbage.

### 7. Soft particle depth fade

**Decision**: The Soft blend pipeline enables depth testing (read-only). The fragment shader receives `u_depth_texture` (the scene depth buffer) and computes: `float depth_fade = saturate((linear_scene_depth - linear_particle_depth) / u_soft_fade_distance)`. The output alpha is multiplied by `depth_fade`. The `u_soft_fade_distance` uniform defaults to 0.5 world units.

**Alternative considered**: Implement depth fade in a post-process pass. Rejected as overly complex for v1.

## Risks / Trade-offs

- **Risk: Instanced rendering compatibility** -- Instanced rendering requires OpenGL 3.3+, which is already the engine's minimum target. No risk.
- **Risk: Free-list memory** -- The free-list adds `max_particles * sizeof(uint32_t)` bytes (~4KB for 1000 particles). Negligible.
- **Risk: Soft particle depth texture binding** -- The scene depth buffer must be available as a texture input to the particle pass. Both forward and deferred renderers already produce a depth buffer that can be sampled. The particle pass must not write to depth while reading it. Current config already has `depth_write = false`.
- **Trade-off: ParticleVertex in shared header** -- Moving `ParticleVertex` to a header creates a new compile dependency. Acceptable since only `particle_renderer.cpp` includes it.

## 1. Fix size_over_life Bug

- [ ] 1.1 Add `float birth_size;` field to `Particle` struct in `engine/runtime/source/scene/component/particle.h`.
- [ ] 1.2 Set `birth_size` at spawn time in `engine/runtime/source/scene/particle_system.cpp` (same random range as `size`).
- [ ] 1.3 Change the `size_over_life` interpolation (line ~210) to compute `p.size = p.birth_size * lerp(size_over_life.x, size_over_life.y, t)` instead of mutating `p.size` in-place.
- [ ] 1.4 Build (`dev\build_vs2026.bat`) and validate shaders (`validate_shaders.bat`).

## 2. Fix scene.cpp Indentation

- [ ] 2.1 Correct indentation of AnimationComponent, ParticleComponent, and ScriptComponent deserialization blocks (lines 392-453) in `engine/runtime/source/scene/scene.cpp` to match the surrounding `for` loop indent level.
- [ ] 2.2 Build to verify no functional change.

## 3. Add YAML Deserialization Error Handling

- [ ] 3.1 Wrap all particle field reads in `scene.cpp` (lines ~406-436) with null/existence checks, matching the pattern used by SpriteComponent and other components.
- [ ] 3.2 Ensure missing fields fall back to `ParticleComponent` default values.
- [ ] 3.3 Build to verify.

## 4. Separate Config from Runtime State

- [ ] 4.1 Define `struct ParticleRuntimeState` in `engine/runtime/source/scene/component/particle.h` containing: `std::vector<Particle> m_particles`, `uint32_t m_alive_count`, `float m_emission_accumulator`, `uint32_t m_total_emitted`, `VertexBuffer m_vbo`, `VertexArray m_vao`, `IndexBuffer m_ibo`, `std::vector<uint32_t> m_free_list`.
- [ ] 4.2 Replace the runtime fields in `ParticleComponent` with `ParticleRuntimeState m_runtime`.
- [ ] 4.3 Update all references in `particle_system.cpp`, `particle_renderer.cpp`, and `scene.cpp`.
- [ ] 4.4 Build and validate.

## 5. Free-list Particle Pool

- [ ] 5.1 Initialize `m_runtime.m_free_list` with all indices `[0, max_particles)` on pool creation.
- [ ] 5.2 Replace the linear dead-slot scan in the spawn loop (`particle_system.cpp:170-178`) with `m_runtime.m_free_list.back()` / `pop_back()`.
- [ ] 5.3 Push dead particle indices back onto the free-list when particles die.
- [ ] 5.4 Build and validate.

## 6. Deduplicate Pipeline Creation

- [ ] 6.1 Create a data-driven helper (table of blend factor configs) in `particle_renderer.cpp` to replace the 3 near-identical pipeline construction blocks.
- [ ] 6.2 Build and validate.

## 7. Extract ParticleVertex to Shared Header

- [ ] 7.1 Move the `ParticleVertex` struct from the lambda in `particle_renderer.cpp:86` to a new location in `particle_renderer.h` (or a dedicated `particle_vertex.h` if more appropriate).
- [ ] 7.2 Update includes and references.
- [ ] 7.3 Build and validate.

## 8. Add `u_has_texture` Uniform

- [ ] 8.1 Add `uniform int u_has_texture;` to `engine/content/shader/particle.glsl`.
- [ ] 8.2 Replace the `tex_color.a < 0.001 && v_texcoord == vec2(0.0)` check with `u_has_texture > 0` branching.
- [ ] 8.3 Set `u_has_texture` in `particle_renderer.cpp` when binding/not binding a texture.
- [ ] 8.4 Build and validate shaders.

## 9. Pass Explicit Camera Vectors

- [ ] 9.1 Add `uniform vec3 u_cam_right;` and `uniform vec3 u_cam_up;` to `particle.glsl`.
- [ ] 9.2 Replace the projview matrix decomposition in the vertex shader with direct use of these uniforms for billboard expansion.
- [ ] 9.3 Set `u_cam_right` and `u_cam_up` in `particle_renderer.cpp` from `CameraComponent` data.
- [ ] 9.4 Build and validate shaders.

## 10. Instanced Quad Rendering

- [ ] 10.1 Create a static shared quad VBO (4 vertices, 6 indices) in `ParticleRenderer`, initialized once.
- [ ] 10.2 Define `ParticleInstanceData` struct (position, size, color, rotation) for per-instance upload.
- [ ] 10.3 Replace the per-particle 4-vertex upload with per-instance data upload to an instance VBO.
- [ ] 10.4 Configure `glVertexAttribDivisor` for instance attributes.
- [ ] 10.5 Replace `glDrawElements` with `glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, alive_count)`.
- [ ] 10.6 Update the vertex shader to expand quads using quad-local vertex position + instance data.
- [ ] 10.7 Build and validate shaders.

## 11. Soft Particle Depth Blending

- [ ] 11.1 Add `uniform sampler2D u_depth_texture;` and `uniform float u_soft_fade_distance;` to `particle.glsl`.
- [ ] 11.2 Implement fragment depth fade: `float depth_fade = saturate((linear_scene_depth - linear_particle_depth) / u_soft_fade_distance);` and multiply output alpha.
- [ ] 11.3 Bind the scene depth buffer as a texture input in `particle_renderer.cpp` when using Soft blend mode.
- [ ] 11.4 Set `u_soft_fade_distance` uniform (default 0.5).
- [ ] 11.5 Build and validate shaders.

## 12. Verification

- [ ] 12.1 Full build: `dev\build_vs2026.bat` succeeds with zero errors.
- [ ] 12.2 Shader validation: `validate_shaders.bat` passes for all shaders.
- [ ] 12.3 Runtime: `engine\bin\Debug\game.exe --frames=10` exits gracefully with code 0.

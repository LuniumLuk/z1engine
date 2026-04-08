## 1. Simulation Correctness Fixes (particle_system.cpp)

- [x] 1.1 Fix `random_direction_in_cone` to normalize the output vector before returning
- [x] 1.2 Add guard in `random_direction_in_cone` for zero-length `base_direction` — if length < epsilon, use fallback `(0, 1, 0)`
- [x] 1.3 Replace linear damping `1.0f - damping * dt` with exponential decay `powf(1.0f - damping, dt)` and remove the `glm::max(0)` clamp
- [x] 1.4 Fix pool resize path: replace `p.lifetime == 0.0f` check with `!p.alive` for dead-particle detection
- [x] 1.5 Add `m_loop` check to emission logic: when `m_loop == false`, stop spawning once `m_total_emitted >= m_max_particles`; set `m_playing = false` when all particles have died
- [x] 1.6 Increment `m_total_emitted` in the spawn loop (continuous emission) to support non-loop budget tracking
- [x] 1.7 Clamp per-entity `dt` to maximum `0.1f` at the top of the per-entity update loop

## 2. Burst Emission (particle.h + particle_system.cpp)

- [x] 2.1 Define `emit_burst(uint32_t count)` method body in `particle_system.cpp` — spawn up to `count` particles from the free list using `spawn_particle`
- [x] 2.2 Add initial burst trigger: after pool initialization, if `m_burst_count > 0`, call `emit_burst(m_burst_count)`
- [x] 2.3 Ensure `emit_burst` increments `m_total_emitted` and `m_alive_count` correctly

## 3. Renderer Robustness (particle_renderer.cpp)

- [x] 3.1 Add VBO resize guard: before uploading instance data, check if the VBO capacity is less than `m_max_particles * sizeof(ParticleInstanceData)` and recreate it if needed
- [x] 3.2 Reset `m_runtime.m_vbo` in the particle pool resize path (particle_system.cpp) so the renderer recreates it on next frame

## 4. Validation and Build

- [x] 4.1 Run `python dev/z1.py compile` and fix any compilation errors
- [x] 4.2 Run `python dev/z1.py smoke` to verify no runtime crashes with existing prefab assets

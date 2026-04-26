## Why

The particle system implementation has several bugs and robustness gaps that can cause incorrect behavior, potential crashes, and subtle rendering artifacts. Key issues include: the `random_direction_in_cone` helper produces non-normalized output causing speed distortion; the `m_loop` flag is never checked so non-looping emitters emit forever; the resize path uses an unreliable heuristic (`lifetime == 0`) to detect dead particles; `m_emission_rate` is read from the wrong field name in the spawn guard; burst emission is declared but never triggered; damping is frame-rate dependent; the instance VBO is never resized if `m_max_particles` grows at runtime; and depth sorting every frame is expensive with no spatial culling or hybrid approach. These issues need to be fixed before the particle system can be considered production-ready.

## What Changes

- **Fix `random_direction_in_cone` normalization**: The helper currently returns `dir + offset` which is not unit-length, causing particles spawned at wider spreads to have inconsistent speeds.
- **Fix non-looping emitter behavior**: `m_loop` is declared and serialized but never checked in `ParticleSystem::update()`. Non-looping emitters emit indefinitely.
- **Fix pool resize dead-particle detection**: The resize path checks `p.lifetime == 0.0f` to detect dead particles instead of checking `p.alive`, which is unreliable (a live particle could have `lifetime == 0` from bad data, and dead particles may have non-zero lifetime).
- **Implement burst emission**: `m_burst_count` is declared, reflected, and serialized, but `emit_burst()` is declared without a definition, and burst logic is never invoked.
- **Fix frame-rate dependent damping**: `1 - damping * dt` is a linear approximation that behaves differently at varying frame rates. Should use exponential decay `pow(1 - damping, dt)` or equivalent.
- **Guard instance VBO against resize**: If `m_max_particles` is increased at runtime, the existing `m_vbo` is too small for the new particle count, causing a GPU buffer overrun on upload.
- **Fix `random_direction_in_cone` zero-vector crash**: If `m_direction` is `(0,0,0)`, `glm::normalize` produces NaN, propagating through the entire spawn path.
- **Add `m_emission_rate` field validation in editor**: The editor min constraint is 0 but there is no max; extremely large values can spawn millions of particles in a single frame after a long pause.
- **Clamp `dt` to prevent emission spike**: A single large `dt` (e.g., after a breakpoint or loading screen) multiplied by emission rate can exhaust the entire pool in one frame.

## Capabilities

### New Capabilities
- `particle-system-robustness`: Covers all bug fixes, guard clauses, numerical stability improvements, and defensive checks across the particle simulation, spawning, and rendering paths.

### Modified Capabilities

## Impact

- `engine/runtime/source/scene/particle_system.cpp` — simulation logic fixes (normalization, loop, resize, damping, burst, dt clamping)
- `engine/runtime/source/scene/component/particle.h` — emit_burst definition or removal of dead declaration
- `engine/runtime/source/render/renderer/particle_renderer.cpp` — VBO resize guard, instance count safety
- `engine/content/shader/particle.glsl` — no shader changes expected
- `engine/runtime/source/scene/scene.cpp` — no serialization changes expected
- All 5 prefab YAML files — no changes expected (config is valid)

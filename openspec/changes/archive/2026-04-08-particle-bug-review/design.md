## Context

The particle system was implemented as a new feature on the `feat-particle` branch. It follows the engine's ECS architecture: `ParticleComponent` holds configuration and runtime state, `ParticleSystem::update()` runs CPU simulation, and `ParticleRenderer` handles GPU instanced rendering via the render graph.

Code review reveals several correctness bugs, a missing feature (burst emission), and robustness gaps that cause incorrect behavior under edge conditions (zero direction vectors, large dt spikes, runtime pool resizes, frame-rate dependent damping). These are not architectural problems — the overall structure is sound — but they are the kind of issues that accumulate into hard-to-diagnose visual glitches and occasional crashes.

Affected files:
- `particle_system.cpp` — simulation logic (5 bugs)
- `particle.h` — dead declaration of `emit_burst()` (1 bug)
- `particle_renderer.cpp` — VBO sizing assumption (1 bug)
- No shader or serialization changes required.

## Goals / Non-Goals

**Goals:**
- Fix all identified correctness bugs (normalization, loop flag, resize detection, damping model)
- Implement or cleanly remove the burst emission feature
- Add defensive guards for degenerate inputs (zero direction, large dt, VBO overflow)
- Ensure all fixes are backward-compatible with existing prefab YAML files

**Non-Goals:**
- Performance optimization (spatial culling, GPU compute simulation, LOD)
- New features (sub-emitters, trails, texture atlas animation, velocity-aligned particles)
- Unit test creation (separate change)
- Shader modifications

## Decisions

### 1. Normalize output of `random_direction_in_cone`

**Problem**: The function returns `dir + random_offset.x * right + random_offset.y * up + (random_offset.z - 1.0f) * dir` which is not unit-length. At wider spreads, the magnitude deviates significantly from 1.0, causing speed distortion.

**Decision**: Normalize the final result before returning. This is the simplest correct fix.

**Alternative considered**: Rewrite using proper spherical cap sampling. Rejected — more complex, the current cone-angle approach is adequate, just needs the output normalized.

### 2. Frame-rate independent damping via exponential decay

**Problem**: `1.0f - damping * dt` is a linear approximation that produces different behavior at 30fps vs 120fps. At high damping values with large dt, the factor can even go negative (clamped to 0, causing instant stop).

**Decision**: Replace with `pow(1.0f - damping, dt)`. This is the mathematically correct continuous-time model and is trivially frame-rate independent. The `glm::max(0)` clamp becomes unnecessary since the exponential is always positive.

**Alternative considered**: Semi-implicit Euler (`exp(-damping * dt)`). Functionally equivalent but less intuitive to artists since the damping parameter meaning changes. The `pow` form preserves the existing parameter semantics: damping=0 means no damping, damping=1 means full stop after 1 second.

### 3. Fix resize path: use `p.alive` instead of `p.lifetime == 0`

**Problem**: When `m_max_particles` changes at runtime, the rebuild logic checks `p.lifetime == 0.0f` as a proxy for "dead particle." This is unreliable: dead particles retain their lifetime from when they were alive, and live particles could theoretically have zero lifetime from bad config.

**Decision**: Check `p.alive` directly. This is the canonical alive/dead flag used everywhere else in the system.

### 4. Implement burst emission (not remove it)

**Problem**: `emit_burst(uint32_t count)` is declared in the header but never defined. `m_burst_count` is reflected and serialized but never consumed.

**Decision**: Implement `emit_burst()` as a method on `ParticleComponent` that immediately spawns `count` particles from the free list. Also add initial-burst support: when the pool is first initialized, if `m_burst_count > 0`, fire an initial burst. This makes the feature usable from the editor and from scripting.

**Alternative considered**: Remove the dead declaration and field entirely. Rejected — burst emission is a common particle system feature, the fields are already in prefab files, and the implementation is trivial.

### 5. Clamp dt to prevent emission spikes

**Problem**: After a breakpoint, loading screen, or scene transition, dt can be very large (e.g., 5+ seconds). `emission_rate * dt` can exceed `m_max_particles`, exhausting the pool in a single frame with all particles born at the same position.

**Decision**: Clamp dt to a maximum of 0.1 seconds (10fps equivalent) at the top of the per-entity update loop. This is a common practice in real-time particle systems.

### 6. Guard zero direction vector

**Problem**: If `m_direction` is `(0,0,0)`, `glm::normalize` returns NaN, which propagates through spawn and corrupts position/velocity.

**Decision**: Add a length check before normalization. If direction length < epsilon, fall back to `(0, 1, 0)` (up). This matches the default value and is the least surprising fallback.

### 7. Recreate VBO when pool size grows

**Problem**: The instance VBO is created once with `m_max_particles * sizeof(ParticleInstanceData)`. If `m_max_particles` is increased at runtime, the VBO is too small, and `write()` overflows the GPU buffer.

**Decision**: When the VBO exists but its size is smaller than needed, reset and recreate it. Check this alongside the existing pool resize logic.

### 8. Check `m_loop` flag to stop emission

**Problem**: `m_loop` is declared, reflected, serialized, and exposed in the editor, but the simulation never checks it. Non-looping emitters behave identically to looping ones.

**Decision**: When `m_loop == false`, track total emitted count against `m_max_particles`. Stop spawning once the budget is exhausted. When all particles have also died, optionally set `m_playing = false` to auto-stop.

## Risks / Trade-offs

- **[Risk] Clamping dt changes visual behavior during slow frames** → Mitigation: 0.1s cap is generous; only triggers during extreme stalls. Standard practice in UE/Unity particle systems.
- **[Risk] `pow(1-damping, dt)` is slightly more expensive than multiply** → Mitigation: One `powf` per particle per frame is negligible vs. the rest of the simulation.
- **[Risk] Burst emission adds a new code path with no tests** → Mitigation: Implementation is straightforward (reuse spawn_particle + free list). Tests are deferred to a separate change.
- **[Risk] Non-loop auto-stop (`m_playing = false`) modifies component state from the system** → Mitigation: This is already the convention in the engine (AnimationSystem sets `playing = false` on completion). Consistent pattern.

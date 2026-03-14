## Why

The particle system shipped in `2026-03-14-particle-system` is functional but contains several code quality issues, a simulation bug, and performance bottlenecks that will compound as the engine gains more particle-heavy scenes. Specifically:

1. **Bug**: The `size_over_life` interpolation mutates `p.size` every frame, losing the birth size and causing exponential shrink instead of a controlled fade.
2. **Code quality**: The `ParticleComponent` conflates emitter configuration (24 reflected fields) with runtime GPU state (VBO/VAO/particle pool) in a single struct. Scene serialization in `scene.cpp` has no error handling for particle fields and broken indentation. Pipeline creation in `particle_renderer.cpp` duplicates identical blocks. The `ParticleVertex` struct is defined inside a render lambda.
3. **Shader fragility**: The particle fragment shader detects missing textures by comparing `texcoord == vec2(0.0)`, and the vertex shader extracts camera vectors by decomposing the projview matrix -- both are brittle and incorrect in edge cases.
4. **Performance**: Each particle uploads 4 vertices per frame (CPU-side quad expansion). Instanced rendering with a single shared quad would cut vertex data by 75%. The spawner does a linear scan for dead slots on every emit. The "Soft" blend mode is identical to Alpha (no actual depth-based soft blending).
5. **Test coverage**: Zero unit tests exist for the particle system.

Addressing these now prevents the issues from becoming entrenched as other systems (prefab instantiation, Python scripting) begin depending on particle APIs.

## What Changes

### Refactors (Bug Fixes & Code Quality)

- **Fix `size_over_life` bug**: Store `birth_size` in the `Particle` struct; interpolate from `birth_size * size_over_life.x` to `birth_size * size_over_life.y` without mutating `p.size` destructively.
- **Fix scene.cpp indentation**: Correct the indentation level of AnimationComponent, ParticleComponent, and ScriptComponent deserialization blocks (lines 392-453).
- **Add YAML deserialization error handling**: Wrap particle field reads in null/existence checks (matching the pattern used by other components like SpriteComponent).
- **Separate config from runtime state**: Split `ParticleComponent` into reflected emitter config fields and a separate `ParticleRuntimeState` struct for GPU buffers, particle pool, alive count, and emission accumulator.
- **Free-list particle pool**: Replace the O(n) linear dead-slot scan in `ParticleSystem::update()` with a free-list that provides O(1) slot allocation.
- **Deduplicate pipeline creation**: Replace 3 near-identical pipeline construction blocks in `ParticleRenderer::init()` with a data-driven helper.
- **Extract `ParticleVertex`**: Move the `ParticleVertex` struct from the render lambda to a shared header for reuse.
- **Add `u_has_texture` uniform**: Replace the fragile `texcoord == vec2(0.0)` texture-missing check in `particle.glsl` with an explicit integer uniform flag.
- **Pass explicit camera vectors**: Pass `u_cam_right` and `u_cam_up` as uniforms instead of decomposing from the projview matrix in the particle vertex shader.

### Enhancements (Performance & Features)

- **Instanced quad rendering**: Use GPU instancing with a single shared quad VBO + per-instance particle data, reducing vertex upload by 75%.
- **Soft particle depth blending**: Implement actual depth-buffer-based soft blending in the Soft pipeline (sample scene depth, compute depth fade factor in fragment shader).

### Deferred

The following are documented as future work but not in scope for this change:
- GPU compute shader simulation
- Texture atlas / sprite sheet animation
- Color-over-life gradient curves
- Sub-emitters
- Velocity-aligned (stretched) billboards
- Unit tests (will be tracked as a separate openspec change)

## Capabilities

### New Capabilities
- `particle-system-quality`: Bug fixes, code quality improvements, and architectural refactoring of the particle system to establish a clean, maintainable foundation.

### Modified Capabilities

## Impact

- **`engine/runtime/source/scene/component/particle.h`**: Struct split, `birth_size` field added to `Particle`, free-list data added.
- **`engine/runtime/source/scene/particle_system.cpp`**: Free-list allocation, fixed size interpolation.
- **`engine/runtime/source/render/renderer/particle_renderer.h/.cpp`**: Pipeline deduplication, `ParticleVertex` extraction, instanced rendering, soft blend depth sampling.
- **`engine/runtime/source/scene/scene.cpp`**: Indentation fix, error handling for particle deserialization.
- **`engine/content/shader/particle.glsl`**: `u_has_texture` uniform, explicit camera vector uniforms, instanced vertex expansion, soft depth fade.
- **Existing behavior**: All changes are backward-compatible. Serialized scene files continue to load. Rendering output should be visually identical (except the size_over_life bug fix, which corrects a visual defect).

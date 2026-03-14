## ADDED Requirements

### Requirement: Correct particle size interpolation over lifetime

The particle system MUST interpolate particle size correctly over its lifetime using the `size_over_life` multiplier range without losing the original birth size.

#### Scenario: Size interpolation preserves birth size
- **WHEN** a particle is spawned with `initial_size` range [0.2, 0.4] and `size_over_life` [1.0, 0.0]
- **THEN** the particle's rendered size at birth MUST equal `birth_size * 1.0` (where `birth_size` is the randomly selected initial size)
- **AND** the particle's rendered size at death MUST equal `birth_size * 0.0`
- **AND** the size MUST interpolate linearly between these values over the particle's lifetime

#### Scenario: Size does not shrink exponentially
- **WHEN** a particle is simulated for multiple frames with `size_over_life` [1.0, 0.5]
- **THEN** the particle size at any time `t` MUST equal `birth_size * lerp(1.0, 0.5, t / lifetime)`
- **AND** the size MUST NOT compound across frames (no exponential decay)

### Requirement: Robust particle YAML deserialization

The particle component YAML deserialization MUST handle missing or malformed fields gracefully without crashing.

#### Scenario: Missing optional fields use defaults
- **WHEN** a scene YAML file contains a `particle` block with some fields missing
- **THEN** the deserialization MUST NOT crash or throw an unhandled exception
- **AND** missing fields MUST use the `ParticleComponent` default values

#### Scenario: Null texture field
- **WHEN** a scene YAML file contains a `particle` block with `texture: ~` (YAML null)
- **THEN** the deserialization MUST set `m_texture` to `nullptr`
- **AND** the particle system MUST render using the default white texture

### Requirement: Separated configuration and runtime state

The `ParticleComponent` MUST separate emitter configuration (reflected, serialized) from transient runtime state (GPU buffers, particle pool, counters).

#### Scenario: Only config fields are reflected
- **WHEN** the editor inspector renders a `ParticleComponent`
- **THEN** only emitter configuration fields (max_particles, emission_rate, colors, etc.) MUST appear
- **AND** runtime state (particle pool, VBO, VAO, alive count, free list) MUST NOT appear

#### Scenario: Only config fields are serialized
- **WHEN** a scene is saved to YAML
- **THEN** only emitter configuration fields MUST be written to the file
- **AND** runtime state MUST NOT appear in the YAML output
- **AND** the saved YAML format MUST be identical to the current format (backward compatible)

### Requirement: O(1) particle slot allocation

The particle system MUST allocate dead particle slots in constant time using a free-list, not a linear scan.

#### Scenario: Spawning particles uses free-list
- **WHEN** particles are emitted during a frame
- **THEN** each spawn MUST pop a slot index from the free-list in O(1) time
- **AND** when a particle dies, its slot index MUST be pushed back onto the free-list

#### Scenario: Free-list initialization
- **WHEN** a particle component's pool is initialized (first frame or max_particles change)
- **THEN** the free-list MUST contain all slot indices [0, max_particles)
- **AND** `alive_count` MUST be 0

### Requirement: Deduplicated pipeline creation

The particle renderer MUST NOT contain duplicated pipeline construction code.

#### Scenario: Pipeline creation via helper
- **WHEN** `ParticleRenderer::init()` creates Alpha, Additive, and Soft blend pipelines
- **THEN** a single helper function or data-driven table MUST be used
- **AND** each pipeline MUST differ only in blend factor configuration

### Requirement: Reliable texture detection in shader

The particle shader MUST use an explicit uniform flag to detect texture availability, not heuristic sampling.

#### Scenario: Texture bound
- **WHEN** `u_has_texture` is set to 1 by the renderer
- **THEN** the fragment shader MUST sample the bound texture and multiply by particle color

#### Scenario: No texture bound
- **WHEN** `u_has_texture` is set to 0 by the renderer
- **THEN** the fragment shader MUST use white (1,1,1,1) as the texture color
- **AND** the shader MUST NOT rely on texcoord values or sampled alpha to detect missing textures

### Requirement: Explicit camera vector uniforms

The particle vertex shader MUST receive camera right and up vectors as explicit uniforms, not extract them from the projection-view matrix.

#### Scenario: Billboard expansion uses explicit uniforms
- **WHEN** the particle vertex shader expands billboard quads
- **THEN** it MUST use `u_cam_right` and `u_cam_up` vec3 uniforms set by the renderer
- **AND** it MUST NOT decompose the projection-view matrix to extract camera axes

### Requirement: Instanced quad rendering

The particle renderer MUST use GPU instancing with a shared quad geometry and per-instance particle data.

#### Scenario: Draw call uses instancing
- **WHEN** particles are rendered
- **THEN** the draw call MUST be `glDrawElementsInstanced` (or equivalent) with instance count equal to alive particle count
- **AND** a single shared quad VBO (4 vertices, 6 indices) MUST be reused across all emitters

#### Scenario: Per-instance data upload
- **WHEN** particle data is uploaded to the GPU each frame
- **THEN** each instance MUST contain exactly one copy of (position, size, color, rotation)
- **AND** the vertex upload size MUST be approximately 25% of the previous 4-vertices-per-particle approach

### Requirement: Depth-based soft particle blending

The Soft blend mode pipeline MUST implement depth-based alpha fade to smooth particle-geometry intersections.

#### Scenario: Soft particles fade near surfaces
- **WHEN** a particle using Soft blend mode is rendered near opaque geometry
- **THEN** the fragment alpha MUST be multiplied by a depth fade factor: `saturate((scene_depth - particle_depth) / fade_distance)`
- **AND** particles far from geometry MUST render at full alpha
- **AND** particles intersecting geometry MUST fade to transparent

### Requirement: Compilation and runtime stability

All changes MUST compile and the engine MUST run without errors.

#### Scenario: Full build
- **WHEN** `dev\build_vs2026.bat` is executed
- **THEN** compilation MUST succeed with zero errors

#### Scenario: Shader validation
- **WHEN** `validate_shaders.bat` is executed
- **THEN** all shaders (including modified `particle.glsl`) MUST pass validation

#### Scenario: Runtime verification
- **WHEN** `engine\bin\Debug\game.exe --one-frame=10` is executed
- **THEN** the engine MUST run 10 frames and exit gracefully with code 0

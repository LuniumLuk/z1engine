## ADDED Requirements

### Requirement: Cone direction sampling produces unit-length output
The `random_direction_in_cone` function SHALL return a normalized (unit-length) vector for all valid spread values in `[0, 1]`.

#### Scenario: Direction at zero spread
- **WHEN** `spread` is `0.0` and `base_direction` is `(0, 1, 0)`
- **THEN** the returned vector SHALL have magnitude `1.0` (within floating-point tolerance of `1e-5`)

#### Scenario: Direction at full spread
- **WHEN** `spread` is `1.0` and `base_direction` is `(0, 1, 0)`
- **THEN** the returned vector SHALL have magnitude `1.0` (within floating-point tolerance of `1e-5`)

#### Scenario: Direction at intermediate spread
- **WHEN** `spread` is `0.5` and `base_direction` is `(1, 0, 0)`
- **THEN** the returned vector SHALL have magnitude `1.0` (within floating-point tolerance of `1e-5`)

### Requirement: Zero direction vector does not produce NaN
The system SHALL handle a zero-length `m_direction` vector without producing NaN or Inf values in particle position or velocity.

#### Scenario: Direction is zero vector
- **WHEN** `m_direction` is `(0, 0, 0)` and a particle is spawned
- **THEN** the particle's velocity SHALL be finite (no NaN or Inf components) and the system SHALL use a fallback direction of `(0, 1, 0)`

### Requirement: Non-looping emitters stop emitting after budget
The system SHALL respect the `m_loop` flag. When `m_loop` is `false`, emission SHALL stop after `m_max_particles` total particles have been emitted over the emitter's lifetime.

#### Scenario: Non-looping emitter reaches budget
- **WHEN** `m_loop` is `false` and `m_total_emitted` reaches `m_max_particles`
- **THEN** no further particles SHALL be spawned by continuous emission

#### Scenario: Looping emitter continues indefinitely
- **WHEN** `m_loop` is `true` and `m_total_emitted` exceeds `m_max_particles`
- **THEN** particles SHALL continue to be spawned as long as free slots exist

#### Scenario: Non-looping emitter auto-stops
- **WHEN** `m_loop` is `false` and `m_total_emitted >= m_max_particles` and `m_alive_count` reaches `0`
- **THEN** `m_playing` SHALL be set to `false`

### Requirement: Pool resize uses alive flag for dead-particle detection
When `m_max_particles` changes at runtime, the resize logic SHALL determine particle liveness using the `alive` field, not `lifetime == 0`.

#### Scenario: Resize with dead particles that have non-zero lifetime
- **WHEN** `m_max_particles` is increased and the pool contains dead particles with `alive == false` and `lifetime > 0`
- **THEN** those particles SHALL be added to the free list

#### Scenario: Resize with live particles that have small lifetime
- **WHEN** `m_max_particles` is increased and the pool contains live particles with `alive == true` and `lifetime` close to zero
- **THEN** those particles SHALL NOT be added to the free list and SHALL continue simulating

### Requirement: Burst emission is functional
The system SHALL support burst emission via the `emit_burst(count)` method and the `m_burst_count` configuration field.

#### Scenario: Manual burst emission
- **WHEN** `emit_burst(10)` is called and the free list has 10 or more slots
- **THEN** exactly 10 particles SHALL be spawned immediately

#### Scenario: Initial burst on pool creation
- **WHEN** the particle pool is first initialized and `m_burst_count > 0`
- **THEN** `m_burst_count` particles SHALL be spawned immediately after initialization

#### Scenario: Burst with insufficient free slots
- **WHEN** `emit_burst(100)` is called and the free list has only 20 slots
- **THEN** exactly 20 particles SHALL be spawned (no overflow, no crash)

### Requirement: Damping is frame-rate independent
Velocity damping SHALL produce consistent results regardless of frame rate. The damping model SHALL use exponential decay: `velocity *= pow(1 - damping, dt)`.

#### Scenario: Equivalent damping at different frame rates
- **WHEN** a particle with `damping = 0.5` and initial speed `10.0` is simulated for 1 second
- **THEN** the final speed SHALL be approximately the same whether simulated at 30fps (33ms steps), 60fps (16ms steps), or 120fps (8ms steps), within 1% tolerance

#### Scenario: Damping of zero has no effect
- **WHEN** `damping` is `0.0`
- **THEN** `pow(1.0 - 0.0, dt)` evaluates to `1.0` and velocity SHALL be unchanged by damping

### Requirement: Large dt is clamped to prevent emission spikes
The system SHALL clamp the per-entity simulation `dt` to a maximum of `0.1` seconds to prevent emission spikes after pauses, breakpoints, or loading screens.

#### Scenario: Normal frame dt
- **WHEN** `dt` is `0.016` (60fps)
- **THEN** the simulation SHALL use `dt = 0.016` unchanged

#### Scenario: Large dt after stall
- **WHEN** `dt` is `2.0` (e.g., after a breakpoint)
- **THEN** the simulation SHALL clamp `dt` to `0.1` before computing emission count and simulation step

### Requirement: Instance VBO is resized when pool grows
The per-component instance VBO SHALL be recreated if `m_max_particles` increases beyond the VBO's current capacity.

#### Scenario: Pool size increases at runtime
- **WHEN** `m_max_particles` is changed from `100` to `500` and the VBO was created for 100 particles
- **THEN** the VBO SHALL be recreated with capacity for 500 particles before the next upload

#### Scenario: Pool size decreases at runtime
- **WHEN** `m_max_particles` is changed from `500` to `100`
- **THEN** the existing VBO MAY be reused (it has sufficient capacity) and no crash SHALL occur

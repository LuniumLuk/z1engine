## ADDED Requirements

### Requirement: Particles attenuate in shadow
Particles that reside within a directional-light shadow cascade SHALL apply a shadow factor to their final colour, darkening the particle in proportion to the CSM depth comparison result.

#### Scenario: Particle inside shadow region appears darker
- **WHEN** a particle billboard is rendered and the CSM depth comparison for its world position returns occluded
- **THEN** the particle's output colour is multiplied by the shadow attenuation factor (< 1.0)

#### Scenario: Particle outside shadow region is unaffected
- **WHEN** a particle billboard is rendered and the CSM depth comparison returns lit
- **THEN** the particle's output colour is unchanged by shadow logic

#### Scenario: Correct cascade is selected
- **WHEN** a particle's view-space depth falls within a specific CSM split range
- **THEN** that cascade's depth map and projection matrix are used for the shadow test

#### Scenario: PCF softening applied
- **WHEN** a shadow test is performed for a particle fragment
- **THEN** a 3×3 PCF kernel is used, matching the softening applied to opaque surfaces

---

### Requirement: Per-emitter shadow reception opt-out
`ParticleComponent` SHALL expose a boolean field `m_receive_shadows` (default `true`). When set to `false`, the particle renderer SHALL skip shadow map binding and shadow sampling for that emitter.

#### Scenario: Shadow reception enabled by default
- **WHEN** a `ParticleComponent` is created with default values
- **THEN** `m_receive_shadows` is `true` and shadow sampling is active at render time

#### Scenario: Shadow reception disabled per emitter
- **WHEN** `m_receive_shadows` is set to `false` on a `ParticleComponent`
- **THEN** the particle renderer does not bind the shadow map for that emitter and particles render at full brightness regardless of shadow coverage

#### Scenario: Additive emitter with shadow reception disabled
- **WHEN** a `ParticleComponent` uses `ParticleBlendMode::Additive` and `m_receive_shadows` is `false`
- **THEN** the emitter renders without any shadow attenuation

---

### Requirement: Shadow attenuation matches scene shadow quality
The shadow factor computed for particles SHALL use the same depth-bias constant and cascade-blending technique applied to opaque surfaces, so particles and scene geometry exhibit consistent shadow boundaries.

#### Scenario: Shadow boundary at cascade split matches opaque surface
- **WHEN** a particle and an adjacent opaque surface both straddle a CSM cascade split plane
- **THEN** both surfaces transition between cascades at the same world-space boundary

---

### Requirement: No shadow casting by particles
Particles SHALL NOT write to the shadow depth map. The shadow pass SHALL continue to render only opaque mesh geometry.

#### Scenario: Particle emitter present during shadow pass
- **WHEN** the shadow pass executes and a scene contains one or more `ParticleComponent` entities
- **THEN** no particle geometry is submitted to the shadow depth framebuffer

## ADDED Requirements

### Requirement: PhysicsComponent enables physics simulation
`PhysicsComponent` SHALL be an ECS component that marks an entity for physics simulation. It SHALL depend on `ColliderComponent` and `TransformComponent`.

#### Scenario: PhysicsComponent requires ColliderComponent and TransformComponent
- **WHEN** an entity has `PhysicsComponent`
- **THEN** that entity also has `ColliderComponent` and `TransformComponent` (enforced via `Requires<T>`)

#### Scenario: Default physics mode is Static
- **WHEN** a `PhysicsComponent` is created with default values
- **THEN** `m_mode` is `PhysicsMode::Static`, `m_mass` is `1.0`, `m_use_gravity` is `true`, `m_linear_damping` is `0.1`

### Requirement: Physics modes
The `PhysicsMode` enum SHALL contain exactly three values: `Static`, `Kinematic`, `Dynamic`.

#### Scenario: Static mode
- **WHEN** `m_mode` is `PhysicsMode::Static`
- **THEN** the entity creates a `PxRigidStatic` actor that collides with dynamic actors but never moves

#### Scenario: Kinematic mode
- **WHEN** `m_mode` is `PhysicsMode::Kinematic`
- **THEN** the entity creates a `PxRigidDynamic` actor with the kinematic flag set; its pose is driven by `TransformComponent` each frame; it pushes dynamic actors but is not affected by forces

#### Scenario: Dynamic mode
- **WHEN** `m_mode` is `PhysicsMode::Dynamic`
- **THEN** the entity creates a `PxRigidDynamic` actor with mass, gravity, and damping; it is affected by forces, collisions, and gravity

### Requirement: PhysicsComponent fields affect dynamic behaviour
The fields `m_mass`, `m_use_gravity`, and `m_linear_damping` SHALL affect PhysicsSystem behaviour for dynamic actors only.

#### Scenario: Dynamic actor with mass 5.0
- **WHEN** a dynamic `PhysicsComponent` has `m_mass = 5.0`
- **THEN** the PhysX rigid body is created with mass 5.0

#### Scenario: Dynamic actor with gravity disabled
- **WHEN** a dynamic `PhysicsComponent` has `m_use_gravity = false`
- **THEN** the PhysX actor does not apply gravity force

#### Scenario: Static actor ignores mass and gravity
- **WHEN** a static `PhysicsComponent` has `m_mass = 10.0` and `m_use_gravity = true`
- **THEN** these values are ignored; the actor remains a `PxRigidStatic` with no mass property

### Requirement: PhysicsComponent serialisation
`PhysicsComponent` fields SHALL be serialised to and deserialised from scene YAML.

#### Scenario: Round-trip serialisation
- **WHEN** a scene is saved with an entity containing `PhysicsComponent`
- **THEN** `m_mode`, `m_mass`, `m_use_gravity`, `m_linear_damping` are written to YAML
- **WHEN** that scene is loaded
- **THEN** all fields are restored to their saved values

### Requirement: PhysicsComponent editor exposure
`PhysicsComponent` fields SHALL be visible and editable in the editor inspector.

#### Scenario: Inspector shows physics fields
- **WHEN** an entity with `PhysicsComponent` is selected
- **THEN** `m_mode` (enum dropdown), `m_mass` (drag), `m_use_gravity` (checkbox), `m_linear_damping` (drag) are shown

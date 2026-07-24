## ADDED Requirements

### Requirement: ColliderComponent defines a collision shape
`ColliderComponent` SHALL be an ECS component that defines a geometric collision shape for an entity, with shape type and dimensions.

#### Scenario: Default collider is a sphere
- **WHEN** a `ColliderComponent` is created with default values
- **THEN** `m_shape` is `ColliderShape::Sphere` and `m_half_extents` is `{0.5, 0.5, 0.5}` (radius = 0.5)

#### Scenario: Box collider with custom half-extents
- **WHEN** `m_shape` is set to `ColliderShape::Box` and `m_half_extents` is set to `{1.0, 2.0, 3.0}`
- **THEN** the collider is a box of half-extents (1.0, 2.0, 3.0) in local space

#### Scenario: Capsule collider with custom radius and half-height
- **WHEN** `m_shape` is set to `ColliderShape::Capsule` and `m_half_extents` is set to `{0.5, 1.5, 0.0}`
- **THEN** the collider is a capsule with radius 0.5 and half-height 1.5

#### Scenario: ColliderComponent serialised with scene
- **WHEN** a scene is saved containing an entity with `ColliderComponent`
- **THEN** `m_shape` and `m_half_extents` are written to the YAML output
- **WHEN** that scene is loaded
- **THEN** the collider shape and dimensions are restored from YAML

#### Scenario: ColliderComponent editable in inspector
- **WHEN** an entity with `ColliderComponent` is selected in the editor
- **THEN** `m_shape` (enum dropdown) and `m_half_extents` (vec3 drag) are visible and editable in the inspector

### Requirement: Collider shapes are limited to Sphere, Box, Capsule
The `ColliderShape` enum SHALL contain exactly three values: `Sphere`, `Box`, `Capsule`.

#### Scenario: ColliderShape enum values
- **WHEN** `ColliderShape` is inspected
- **THEN** it has exactly `Sphere = 0`, `Box = 1`, `Capsule = 2`

### Requirement: ColliderComponent does not imply physics simulation
`ColliderComponent` alone SHALL NOT cause an entity to participate in physics simulation. Only entities with both `ColliderComponent` and `PhysicsComponent` SHALL be simulated.

#### Scenario: Entity with only ColliderComponent
- **WHEN** an entity has `ColliderComponent` but no `PhysicsComponent`
- **THEN** the entity does not create a PhysX actor and is not included in the physics step

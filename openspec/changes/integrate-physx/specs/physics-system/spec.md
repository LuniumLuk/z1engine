## ADDED Requirements

### Requirement: PhysicsSystem owns a PhysX scene
`PhysicsSystem` SHALL own and manage a PhysX `PxScene` with default gravity `(0, -9.81, 0)` and a single-threaded CPU dispatcher.

#### Scenario: PhysicsSystem initialisation
- **WHEN** `PhysicsSystem::init()` is called
- **THEN** a `PxPhysics` instance, a `PxDefaultCpuDispatcher`, a `PxCooking` instance, and a `PxScene` with gravity `(0, -9.81, 0)` are created and ready

#### Scenario: PhysicsSystem shutdown
- **WHEN** `PhysicsSystem::shutdown()` is called
- **THEN** all PhysX objects are released in reverse creation order with no leaks

### Requirement: PhysicsSystem steps at fixed timestep
`PhysicsSystem::update(Scene*, float dt)` SHALL accumulate the frame delta and step the PhysX scene at a fixed timestep of 1/60s.

#### Scenario: 1/30s frame (two physics steps)
- **WHEN** `update()` is called with `dt = 1.0/30.0`
- **THEN** the accumulator reaches > 1/60; `PxScene::simulate(1/60)` is called twice

#### Scenario: 1/120s frame (< 1/60 dt)
- **WHEN** `update()` is called with `dt = 1.0/120.0`
- **THEN** the accumulator accumulates; no `simulate()` call occurs on this frame

### Requirement: Entity→actor synchronisation
`PhysicsSystem` SHALL create PhysX actors for entities with both `ColliderComponent` and `PhysicsComponent`, update kinematic actor poses each frame, read back dynamic actor poses, and remove actors for stale entities.

#### Scenario: Actor creation
- **WHEN** an entity with `ColliderComponent` + `PhysicsComponent` (Static mode) is present
- **THEN** a `PxRigidStatic` with the appropriate `PxShape` is created at the entity's world transform

#### Scenario: Kinematic pose sync
- **WHEN** an entity has a kinematic `PhysicsComponent` and its `TransformComponent` changes
- **THEN** the PhysX actor's global pose is updated to match before `simulate()` is called

#### Scenario: Dynamic pose readback
- **WHEN** a dynamic actor moves due to physics simulation
- **THEN** after `fetchResults()`, the actor's global pose is copied back to the entity's `TransformComponent`

#### Scenario: Stale actor cleanup
- **WHEN** an entity that previously had `ColliderComponent` + `PhysicsComponent` loses one of them or is destroyed
- **THEN** its PhysX actor is released and removed from the tracking map

### Requirement: Collider shape mapping
`PhysicsSystem` SHALL create PhysX geometry from `ColliderComponent` as follows:
- `Sphere` → `PxSphereGeometry(radius = m_half_extents.x)`
- `Box` → `PxBoxGeometry(halfX = m_half_extents.x, halfY = m_half_extents.y, halfZ = m_half_extents.z)`
- `Capsule` → `PxCapsuleGeometry(radius = m_half_extents.x, halfHeight = m_half_extents.y)`

#### Scenario: Sphere shape creation
- **WHEN** `ColliderComponent` has `m_shape = Sphere` with `m_half_extents.x = 2.0`
- **THEN** a `PxSphereGeometry(2.0)` is created

#### Scenario: Box shape creation
- **WHEN** `ColliderComponent` has `m_shape = Box` with `m_half_extents = {1.0, 2.0, 3.0}`
- **THEN** a `PxBoxGeometry(1.0, 2.0, 3.0)` is created

#### Scenario: Capsule shape creation
- **WHEN** `ColliderComponent` has `m_shape = Capsule` with `m_half_extents = {0.5, 1.5, 0.0}`
- **THEN** a `PxCapsuleGeometry(0.5, 1.5)` is created

### Requirement: PhysicsSystem registered on RuntimeContext
`PhysicsSystem` SHALL be a member of `RuntimeContext`, initialised in `RuntimeContext::init()` and shut down in `RuntimeContext::shutdown()`.

#### Scenario: RuntimeContext access
- **WHEN** `g_runtime_context` is initialised
- **THEN** `g_runtime_context.m_physics_system` is valid and ready

### Requirement: PhysicsSystem ticked from Scene::on_update
`Scene::on_update()` SHALL call `PhysicsSystem::update()` each frame.

#### Scenario: Scene update calls physics
- **WHEN** `Scene::on_update(dt)` is called
- **THEN** `g_runtime_context.m_physics_system->update(this, dt)` is called before other systems that read transforms

### Requirement: PhysX errors logged to engine log
`PhysicsSystem` SHALL register a `PxErrorCallback` that forwards PhysX error/warning messages to the z1engine logger.

#### Scenario: PhysX error
- **WHEN** PhysX reports an error via `PxErrorCallback::reportError`
- **THEN** the message is logged via `Z1_LOG_ERROR` or `Z1_LOG_WARN`

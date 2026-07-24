## Why

z1engine currently has no physics simulation — entities are static, and any movement is purely kinematic (scripted or animated). A minimal physics integration enables collision detection, rigid body dynamics, and environmental interaction (gravity, forces, triggers) — foundational for gameplay, editor snapping, and future features like character controllers or vehicles.

## What Changes

- **PhysX SDK** integrated as a third-party library under `engine/3rdparty/physx/` (manual fetch & place; this proposal includes placement guide)
- **`ColliderComponent`** — attachable to entities to define collision shapes: sphere, box, capsule
- **`PhysicsComponent`** — marks an entity for physics simulation; requires `ColliderComponent` + `TransformComponent`; three modes: static, kinematic, dynamic
- **`PhysicsSystem`** — runtime system that creates a PhysX world, synchronises ECS transforms ↔ PhysX actors, steps the simulation each frame, and is registered on `RuntimeContext`
- No editor UI in this initial change — components are configurable via inspector reflection only

## Capabilities

### New Capabilities

- `collider-component`: `ColliderComponent` defines a collision shape (sphere/box/capsule) with per-shape params (radius, half-extents). Serialised/reflected for editor inspector.
- `physics-component`: `PhysicsComponent` attaches physics behaviour (static/kinematic/dynamic) with mass, gravity flag, and linear damping. Requires `ColliderComponent` and `TransformComponent`.
- `physics-system`: `PhysicsSystem` owns a PhysX `PxScene`, creates/destroys actors per entity, syncs transform each frame, steps simulation at fixed timestep. Registered on `RuntimeContext`.

## Impact

- `engine/3rdparty/physx/` — PhysX SDK headers + libs (manual placement; guide provided)
- `engine/3rdparty/physx/premake5.lua` — build integration for PhysX static libs
- `premake5.lua` — add physx to dependency group
- `engine/runtime/premake5.lua` — link physx, add include dirs
- `engine/runtime/source/scene/component/collider.h` — new ColliderComponent
- `engine/runtime/source/scene/component/physics.h` — new PhysicsComponent
- `engine/runtime/source/scene/physics_system.h/.cpp` — new PhysicsSystem
- `engine/runtime/source/core/reflection_hooks.cpp` — register component hooks
- `engine/runtime/source/scene/scene.cpp` — call PhysicsSystem::update in on_update
- `engine/runtime/source/core/core.h` — add PhysicsSystem to RuntimeContext
- `engine/runtime/source/core/core.cpp` — init/shutdown PhysicsSystem

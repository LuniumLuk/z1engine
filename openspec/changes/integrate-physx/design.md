## Context

z1engine uses EnTT ECS with hand-rolled macro-driven static reflection (`REFLECTED_COMPONENT` / `REFLECTED_FIELD`). Components are plain structs with field reflection outside the struct body. Runtime systems are simple static-method structs called from `Scene::on_update()` — there is no system registry, no base class, no dependency injection. Third-party libraries live under `engine/3rdparty/<lib>/` with optional `premake5.lua` for compiled libs.

The engine has no physics simulation today. `TransformComponent` holds position/rotation/scale but is never driven by external forces or collision constraints. This proposal adds the minimal PhysX binding to fill that gap.

**PhysX version**: NVIDIA PhysX 5.6 (latest open-source release). PhysX 5 dropped the custom memory allocator requirement — it uses standard `malloc`/`free` by default, simplifying integration.

## Goals / Non-Goals

**Goals:**
- PhysX SDK as a third-party dependency (manual placement; premake5 integration provided)
- `ColliderComponent` with sphere, box, capsule shapes (physically meaningful colliders, not mesh colliders)
- `PhysicsComponent` with static/kinematic/dynamic modes; requires `ColliderComponent` + `TransformComponent`
- `PhysicsSystem` that creates/manages a `PxScene`, syncs transforms, and steps the physics world
- Keep the initial integration minimal: no joints, no triggers, no character controllers, no scene queries exposed to scripting

**Non-Goals:**
- Mesh/convex colliders — deferred to a follow-up change
- Physics material properties (friction, restitution) — use PhysX defaults
- Trigger volumes / overlap events — deferred
- Editor gizmos for collider shapes — inspector-only at this stage
- Multi-scene physics worlds — one scene, one PhysX world
- Networking/replication of physics state
- Continuous collision detection (CCD)

## Decisions

### PhysX version: 5.6

**Decision**: Use PhysX 5.6 (open-source, MIT-licensed).

**Rationale**: PhysX 5 is a major rewrite over PhysX 4 — it drops `PxFoundation`/`PxPhysics` singletons, uses simpler `PxCreatePhysics()`, and no longer requires a custom allocator callback. The API surface for basic rigid bodies + colliders is minimal. PhysX 5.6 is the latest stable open-source release.

**Alternative considered**: PhysX 4.1. Rejected — PhysX 4 requires a custom allocator and `PxFoundation` singleton, adding boilerplate. PhysX 5 is simpler for a first integration.

---

### Component split: ColliderComponent + PhysicsComponent

**Decision**: Two separate components rather than one merged `RigidbodyComponent`.

**Rationale**: Separating shape from physics behaviour follows the PhysX data model (`PxShape` is a child of `PxRigidActor`) and allows future flexibility:
- Colliders without physics (e.g., trigger-only volumes, editor-only snapping)
- Multiple colliders on one physics body (compound shapes) in a future change
- A `PhysicsComponent` explicitly declares "this entity participates in simulation" rather than silently inferring from collider presence

**Alternative considered**: Single `RigidbodyComponent` with embedded shape. Rejected — conflates shape and behaviour, makes multi-collider or physics-free collider workflows impossible.

---

### PhysicsComponent::m_mode enum: Static, Kinematic, Dynamic

**Decision**: Three-mode enum matching PhysX actor types.

- **Static** (`PxRigidStatic`): Never moves. Collides with dynamic actors. Use for floors, walls, terrain.
- **Kinematic** (`PxRigidDynamic` with kinematic flag): Moved by script/animation. Pushes dynamic actors out of its way. Not affected by forces.
- **Dynamic** (`PxRigidDynamic`): Full physics simulation — gravity, forces, collisions, velocity.

**Rationale**: These three cover the essential gameplay cases. Static is the default (most common use: environment colliders). Kinematic enables script-driven platforms/doors. Dynamic enables physics-driven objects.

---

### PhysicsSystem owns PxScene; no separate PhysicsWorld wrapper

**Decision**: `PhysicsSystem` directly owns `PxScene*` (and `PxPhysics*` / `PxCooking*` / `PxDefaultCpuDispatcher*`). No intermediate `PhysicsWorld` abstraction class.

**Rationale**: Keep it simple. A wrapper class adds indirection without value at this stage. If multi-scene support is needed later, the PhysX management can be refactored into a `PhysicsWorld` owned by the `PhysicsSystem`.

---

### Transform synchronisation: ECS → PhysX each frame, PhysX → ECS for dynamics only

**Decision**: 
- For **static** actors: set PhysX pose once at creation, never sync back
- For **kinematic** actors: copy ECS `TransformComponent` → PhysX each frame before `simulate()`
- For **dynamic** actors: copy PhysX global pose → ECS `TransformComponent` after `fetchResults()`

**Rationale**: Matches PhysX semantics. Static actors never move. Kinematic actors are driven by the application. Dynamic actors are driven by the physics solver. Reading back dynamic transforms ensures physics-driven movement is visible to the renderer and other ECS systems.

---

### Fixed timestep with accumulator

**Decision**: Physics ticks at a fixed 1/60s timestep with an accumulator. `PhysicsSystem::update(Scene*, float dt)` accumulates `dt`, steps `PxScene::simulate(1/60)` in a loop until the accumulator is exhausted.

**Rationale**: Fixed timestep ensures deterministic, stable simulation regardless of frame rate. The accumulator pattern is standard in game physics (see: Glenn Fiedler's "Fix Your Timestep").

---

### PhysX placement guide (manual fetch)

**Decision**: PhysX is NOT committed to the repo or fetched automatically. The proposal includes a placement guide. The developer manually downloads the PhysX 5.6 SDK and places files in `engine/3rdparty/physx/`.

**Rationale**: PhysX SDK is ~100MB+ and platform-specific. Keeping it out of the repo avoids bloat. The premake5.lua is pre-written so that once the files are placed, the build just works.

**Placement guide structure**: See `tasks.md` Task 1 for detailed steps.

---

### No PVD (PhysX Visual Debugger) in initial integration

**Decision**: Do not connect PVD in this change.

**Rationale**: PVD requires extra setup (`PxPvdTransport`, connection flags) and adds complexity. Collider visualisation can be done in-editor via ImGui wireframe rendering in a follow-up. For debugging, PhysX error callbacks to the engine log are sufficient.

## Risks / Trade-offs

- **[Risk] PhysX binary compatibility** — PhysX 5.6 prebuilt libs may not match VS2026 toolchain (v145).  
  → **Mitigation**: The placement guide instructs building PhysX from source with the same VS version. The premake5.lua assumes static libs built from source.

- **[Risk] Coordinate system mismatch** — PhysX uses left-handed Y-up; z1engine uses left-handed Y-up (OpenGL convention). Scale differs (PhysX: metres; z1engine: arbitrary units).  
  → **Mitigation**: Match the coordinate systems directly (both Y-up). PhysX default gravity is `(0, -9.81, 0)`. Scale is inherently arbitrary — authors tune values as needed.

- **[Risk] PhysX allocator thread safety** — PhysX internal allocations may not be thread-safe with EnTT iteration.  
  → **Mitigation**: Physics runs single-threaded in the main thread via `Scene::on_update()`. No concurrent access.

- **[Risk] Entity lifecycle divergence** — If an entity is destroyed in ECS but PhysX still holds its actor, we have a dangling pointer.  
  → **Mitigation**: `PhysicsSystem` stores `entt::entity → PxRigidActor*` in a map. On each frame, clean up actors whose ECS entity no longer exists. In a future change, use EnTT `on_destroy` signals for immediate cleanup.

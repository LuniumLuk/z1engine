## 1. PhysX Third-Party Integration (Manual Placement)

### 1.1 Fetch and place PhysX SDK
- [ ] 1.1.1 Clone or download PhysX 5.6 source from: `https://github.com/NVIDIA-Omniverse/PhysX` (branch: `release-5.6.0` or latest 5.x stable tag)
- [ ] 1.1.2 Build PhysX from source for Windows x64, VS2026 toolchain (v145), Release and Debug configs. Follow the PhysX build guide — typically:
  ```
  cd physx/physx
  generate_projects.bat
  # Build physx, physxcommon, physxcooking, physxextensions, physxfoundation, physxpvd
  ```
- [ ] 1.1.3 Place the built output under `engine/3rdparty/physx/`:
  ```
  engine/3rdparty/physx/
    include/
      PxPhysicsAPI.h
      px/          (or physx/)
      ...
    lib/
      Debug/
        PhysX_static_64.lib
        PhysXCommon_static_64.lib
        PhysXCooking_static_64.lib
        PhysXExtensions_static_64.lib
        PhysXFoundation_static_64.lib
      Release/
        PhysX_static_64.lib
        PhysXCommon_static_64.lib
        PhysXCooking_static_64.lib
        PhysXExtensions_static_64.lib
        PhysXFoundation_static_64.lib
  ```
  **Note**: Exact lib names depend on the PhysX build config. The premake5.lua (Task 1.2) will be adjusted to match.

### 1.2 Build-system integration
- [ ] 1.2.1 Create `engine/3rdparty/physx/premake5.lua`:
  - Define a `physx` static library project that wraps the prebuilt `.lib` files
  - Use `filter "configurations:Debug"` / `filter "configurations:Release"` to select debug/release lib paths
  - Set `includedirs` to `include/`
- [ ] 1.2.2 Add `include "engine/3rdparty/physx"` to the `group "dependency"` section of root `premake5.lua`
- [ ] 1.2.3 Add `"physx"` to `links {}` in `engine/runtime/premake5.lua`
- [ ] 1.2.4 Add `"%{wks.location}/engine/3rdparty/physx/include"` to `includedirs {}` in `engine/runtime/premake5.lua`
- [ ] 1.2.5 Add `PX_PHYSX_STATIC_LIB` (or equivalent) to `defines {}` in `engine/runtime/premake5.lua` if PhysX static linking requires it

## 2. ColliderComponent

- [ ] 2.1 Create `engine/runtime/source/scene/component/collider.h`:
  ```cpp
  // Enum: ColliderShape { Sphere, Box, Capsule }
  // REFLECTED_COMPONENT(ColliderComponent):
  //   ColliderShape m_shape = ColliderShape::Sphere
  //   glm::vec3 m_half_extents = {0.5, 0.5, 0.5}  // box half-size / sphere radius stored in x / capsule radius in x, half-height in y
  ```
  - Sphere: `m_half_extents.x` = radius
  - Box: `m_half_extents` = half-extents (all three axes)
  - Capsule: `m_half_extents.x` = radius, `m_half_extents.y` = half-height (cylinder portion, excluding caps)
- [ ] 2.2 Add `REFLECTED_FIELD` declarations for `m_shape` and `m_half_extents` with appropriate widget hints (`[drag]min=0.01`)
- [ ] 2.3 Add `DISABLE_COPY(ColliderComponent)`
- [ ] 2.4 Register `REGISTER_COMPONENT_HOOKS(ColliderComponent)` in `reflection_hooks.cpp`
- [ ] 2.5 Add `#include "scene/component/collider.h"` to `reflection_hooks.cpp`

## 3. PhysicsComponent

- [ ] 3.1 Create `engine/runtime/source/scene/component/physics.h`:
  ```cpp
  // Enum: PhysicsMode { Static, Kinematic, Dynamic }
  // REFLECTED_COMPONENT(PhysicsComponent) : Requires<ColliderComponent, TransformComponent>:
  //   PhysicsMode m_mode = PhysicsMode::Static
  //   float m_mass = 1.0f
  //   bool m_use_gravity = true
  //   float m_linear_damping = 0.1f
  ```
  - `m_mass` is ignored for Static/Kinematic modes
  - `m_use_gravity` is ignored for Static/Kinematic modes
- [ ] 3.2 Add `REFLECTED_FIELD` declarations with appropriate widget hints (`[drag]min=0.0` for mass/damping)
- [ ] 3.3 Add `DISABLE_COPY(PhysicsComponent)`
- [ ] 3.4 Register `REGISTER_COMPONENT_HOOKS(PhysicsComponent)` in `reflection_hooks.cpp`
- [ ] 3.5 Add `#include "scene/component/physics.h"` to `reflection_hooks.cpp`

## 4. PhysicsSystem

- [ ] 4.1 Create `engine/runtime/source/scene/physics_system.h`:
  ```cpp
  struct PhysicsSystem {
      void init();
      void shutdown();
      void update(Scene* scene, float dt);
      // ... private: PxPhysics*, PxScene*, PxDefaultCpuDispatcher*, PxCooking*, entt::entity→actor map
  };
  ```
- [ ] 4.2 Create `engine/runtime/source/scene/physics_system.cpp`:
  - `init()`: Create `PxPhysics` (via `PxCreatePhysics()`), `PxScene` with default gravity `(0, -9.81, 0)`, `PxDefaultCpuDispatcher`, `PxCooking`
  - `shutdown()`: Release all PhysX objects in reverse order
  - `update(Scene*, float dt)`: Fixed-timestep accumulator at 1/60s:
    1. Sync ECS entities → PhysX actors (create new, update existing, remove stale)
    2. For kinematic actors: copy `TransformComponent` world transform → PhysX
    3. `PxScene::simulate(fixed_dt)` + `PxScene::fetchResults(true)`
    4. For dynamic actors: copy PhysX global pose → `TransformComponent`
- [ ] 4.3 Actor creation logic:
  - Static mode → `PxRigidStatic` + `PxShape` (from `ColliderComponent` shape)
  - Kinematic mode → `PxRigidDynamic` with `setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true)` + `PxShape`
  - Dynamic mode → `PxRigidDynamic` with mass, gravity, damping + `PxShape`
- [ ] 4.4 Shape creation from `ColliderComponent`:
  - Sphere → `PxSphereGeometry(m_half_extents.x)`
  - Box → `PxBoxGeometry(m_half_extents.x, m_half_extents.y, m_half_extents.z)`
  - Capsule → `PxCapsuleGeometry(m_half_extents.x, m_half_extents.y)`
- [ ] 4.5 Stale-entity cleanup: iterate actor map, remove actors whose ECS entity no longer exists or no longer has both `ColliderComponent` + `PhysicsComponent`
- [ ] 4.6 Add error callback: register a `PxErrorCallback` that logs to the engine logger

## 5. RuntimeContext & Scene Integration

- [ ] 5.1 Add `std::shared_ptr<PhysicsSystem> m_physics_system;` to `RuntimeContext` in `core/core.h`
- [ ] 5.2 In `RuntimeContext::init()` (`core/core.cpp`): create and init `m_physics_system`
- [ ] 5.3 In `RuntimeContext::shutdown()` (`core/core.cpp`): shutdown `m_physics_system` before other teardown
- [ ] 5.4 In `Scene::on_update()` (`scene/scene.cpp`): call `g_runtime_context.m_physics_system->update(this, delta_time)` — after `TransformComponent` previous-frame save, before other systems that might read transforms
- [ ] 5.5 Add `#include "scene/physics_system.h"` to `core/core.h` and/or `scene/scene.cpp` as needed

## 6. Validation

- [ ] 6.1 Build the project: `python dev/z1.py compile` — ensure physx links without errors
- [ ] 6.2 Smoke-test: `python dev/z1.py smoke` — verify editor loads without crash
- [ ] 6.3 Manual test: add `ColliderComponent` + `PhysicsComponent` to an entity in the editor inspector, verify no crash
- [ ] 6.4 Manual test: run editor with a dynamic body (box + sphere), verify it falls under gravity
- [ ] 6.5 Manual test: place a static box collider, drop a dynamic sphere on it, verify collision

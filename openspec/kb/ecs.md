# Entity Component System
> Summary: ECS architecture using EnTT, scenes, components, and systems
> Scope: engine/runtime/source/scene/, engine/runtime/source/scene/component/

## Core Architecture

- **ECS library**: EnTT (header-only, in `engine/3rdparty/`)
- **Scene**: owns an EnTT registry, represents a world of entities
- **Entity**: lightweight ID wrapper around EnTT entity
- **Components**: `REFLECTED_STRUCT` data types attached to entities
- **Systems**: logic that operates on entities with specific components

## Key Types

| Type | Header | Role |
|------|--------|------|
| `Scene` | `scene/scene.h` | World container, inherits `Asset<Scene>` |
| `Entity` | `scene/entity.h` | Entity handle wrapper |
| `ScriptBase` | `scene/component/base.h` | Base class for script behaviors |
| `EditorCameraData` | `scene/scene.h` | Editor camera state (nested) |

## Components

| Component | Header | Data |
|-----------|--------|------|
| `TagComponent` | `component/base.h` | Entity name/tag |
| `TransformComponent` | `component/base.h` | Position, rotation, scale |
| `ScriptComponent` | `component/base.h` | Script module + class name |
| `CameraComponent` | `component/camera.h` | Camera properties (requires `TransformComponent`) |
| `LightComponent` | `component/light.h` | Light type, color, intensity |
| `StaticMeshComponent` | `component/mesh.h` | Static mesh reference |
| `SkeletalMeshComponent` | `component/mesh.h` | Animated mesh reference |
| `SpriteComponent` | `component/sprite.h` | 2D sprite data |
| `SkyLightComponent` | `component/sky_light.h` | Environment lighting |
| `PostprocessVolumeComponent` | `component/postprocess_volume.h` | Post-processing settings |
| `AnimationComponent` | `component/animation.h` | Animation state |

## Systems

| System | File | Role |
|--------|------|------|
| `AnimationSystem` | `scene/animation_system.h/.cpp` | Updates skeletal animations |
| `PostprocessSystem` | `scene/postprocess_system.h/.cpp` | Manages post-process volumes |
| `ScriptSystem` | `scene/script_system.h/.cpp` | Executes script `on_update` callbacks |

## Light Types

- `LightType::Directional` -- sun/directional light
- `LightType::Point` -- point light
- `LightType::Spot` -- spotlight

## Scene Lifecycle

- `Scene::on_update(dt)` -- ticks all systems, iterates entities
- `Scene::on_runtime_start()` / `on_runtime_stop()` -- lifecycle boundaries
- Scene serialization: YAML format via `scene.cpp`

## Prefabs

- `Prefab` (`scene/prefab.h`) -- reusable entity templates
- Can be instantiated into a scene

## Reflection

- Components use `REFLECTED_STRUCT` macro for serialization and editor inspection
- `Requires<T>` template declares component dependencies

-> see [python-scripting.md]
-> see [architecture.md]

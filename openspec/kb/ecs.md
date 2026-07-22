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
| `ParticleComponent` | `component/particle.h` | GPU particle emitter: rate, lifetime, cone angle, damping, burst, loop, gravity, **receive_shadows**, **cast_shadows** |

## Systems

| System | File | Role |
|--------|------|------|
| `AnimationSystem` | `scene/animation_system.h/.cpp` | Updates skeletal animations |
| `PostprocessSystem` | `scene/postprocess_system.h/.cpp` | Manages post-process volumes |
| `ScriptSystem` | `scene/script_system.h/.cpp` | Executes script `on_update` callbacks |
| `ParticleSystem` | `scene/particle_system.h/.cpp` | Spawns, simulates, and kills GPU particles per entity |

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

- Components use `REFLECTED_COMPONENT` macro (expands to `REFLECTED_STRUCT` + ECS hook registration)
- `REFLECTED_FIELD` registers fields with flags, widget hints, and YAML key overrides
- `TypeInfo` carries type-erased hooks: `add_to(Entity&)`, `remove_from(Entity&)`, `has_in(Entity const&)`
- Component hooks are registered in `core/reflection_hooks.cpp` (deferred to after `Entity` is fully defined)
- `TypeRegistry::get_all_components()` returns all types with `add_to` hooks — used by editor menus
- `Requires<T>` template declares component dependencies
- Serialization is now reflection-driven via `scene/serialization.h`: `serialize_type()` / `deserialize_type()`
- Field-level flags control behavior: `FF_Default` (visible+editable+serializable), `FF_ReadOnly` (visible only)
- Asset reference fields (`shared_ptr<AssetT>`) are auto-detected via `is_asset_type<T>` trait
- `yaml_key` override on `FieldInfo` maps C++ member names to legacy YAML key names
- Enum reflection via `REFLECT_ENUM` — values accessible in editor and serialization

-> see [python-scripting.md]
-> see [architecture.md]

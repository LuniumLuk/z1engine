## ADDED Requirements

### Requirement: Minimal macro-based registration model is preserved

The reflection system SHALL remain a header-macro plus static-registration design: `REFLECTED_STRUCT`, `REFLECTED_FIELD`, and `REFLECT_ENUM` in `engine/runtime/source/core/core.h`, with metadata stored in `TypeRegistry` / `EnumRegistry` (`engine/runtime/source/core/reflection.h`). No external reflection library SHALL be introduced, and existing declaration sites SHALL keep compiling unchanged.

#### Scenario: Existing reflected types keep working

- **WHEN** the engine starts after the upgrade
- **THEN** all types reflected before the upgrade (e.g. `TransformComponent`, `LightComponent`, `ParticleComponent`, `GlobalSettings`) are still registered with identical field names, offsets, and flags

#### Scenario: Registration completeness self-test

- **WHEN** the runtime test executable runs
- **THEN** it asserts every type expected by the engine (all `*Component` types and `GlobalSettings`) is present in `TypeRegistry`
- **AND** it fails if linker stripping removed any registrar

---

### Requirement: Component attach hooks

`TypeInfo` SHALL support optional type-erased hooks registered via a new `REFLECTED_COMPONENT(Type)` macro: default-construct, and for components add/remove/has on an `Entity`. Non-component reflected structs SHALL NOT require hooks.

#### Scenario: Add component generically

- **WHEN** a consumer (serializer, editor, Python binding) requests adding a component by type name
- **THEN** the registry hook attaches the component to the given entity with default field values
- **AND** requesting an unknown or hook-less type name fails with a clear error

#### Scenario: Enumerate attachable components

- **WHEN** a consumer lists all registered types that carry component hooks
- **THEN** every engine component (`TagComponent`, `TransformComponent`, `CameraComponent`, `LightComponent`, `StaticMeshComponent`, `SkeletalMeshComponent`, `SpriteComponent`, `SkyLightComponent`, `PostprocessVolumeComponent`, `AnimationComponent`, `ParticleComponent`, `ScriptComponent`) is included

---

### Requirement: Guid and asset-reference fields

The reflection metadata SHALL support `Guid` fields and asset-reference fields (`std::shared_ptr<T>` where `T` is an engine asset type listed in a single asset-type trait in `reflection.h`).

#### Scenario: Tag id reflected

- **WHEN** `TagComponent` metadata is queried
- **THEN** `m_id` is present as a `Guid` field with read-only editor flag and serializable flag

#### Scenario: Mesh asset references reflected

- **WHEN** `StaticMeshComponent` and `SkeletalMeshComponent` metadata is queried
- **THEN** their mesh fields are present and recognized as asset references of the corresponding asset type

---

### Requirement: Custom field accessors

`FieldInfo` SHALL support an optional pair of type-erased get/set callbacks for fields that cannot be modeled as offset+typeid+size. Consumers (editor, serializer, generator) SHALL use the callbacks when present and the offset path otherwise.

#### Scenario: Material variable value access

- **WHEN** the value of a `Material::Variable` is read or written through reflection
- **THEN** the custom accessor converts between the tagged-union storage and a typed view (float/int/vector/texture) selected by `DataType`

#### Scenario: Script entries access

- **WHEN** `ScriptComponent` metadata is queried
- **THEN** its script entries are exposed as an editable list of module/class string pairs backed by the runtime script data

---

### Requirement: Engine-wide reflected type coverage

Types that are shown in the editor, serialized, or used by Python scripts SHALL be reflected. This includes at minimum: `TagComponent::m_id`, `StaticMeshComponent::m_mesh`, `SkeletalMeshComponent::m_mesh`, `ScriptComponent` script entries, `Material::Variable`, `Material::m_variables`, `MaterialInstance::m_override_variables`, `MaterialInstance::m_material`, the `DataType` enum, and `Scene::EditorCameraData`.

#### Scenario: No hand-maintained mirror data

- **WHEN** a developer adds a field to a reflected struct with `FF_Default`
- **THEN** the field appears in the editor inspector and in scene save output without edits to `scene.cpp`, `type_field.cpp`, or `py_engine.cpp`

---

### Requirement: Reflection-driven scene serialization

Scene load/save in `engine/runtime/source/scene/scene.cpp` SHALL serialize components, `global_settings`, and `editor_camera` by iterating reflection metadata and honoring `FF_Serializable`, replacing the per-component hand-written YAML blocks. The on-disk YAML schema SHALL remain compatible with scenes written before the upgrade.

#### Scenario: Round-trip stability

- **WHEN** a scene is loaded and saved again
- **THEN** all reflected `FF_Serializable` fields retain their values
- **AND** `engine/test/test_scene_serialize.cpp` passes

#### Scenario: Existing scene files still load

- **WHEN** `engine/content/scene/ParticleDemo.yaml` (written before the upgrade) is loaded
- **THEN** every entity and component deserializes with identical values
- **AND** re-saving produces the same set of YAML keys for each component

#### Scenario: Non-serializable fields stay out

- **WHEN** a field is declared without `FF_Serializable` (e.g. runtime particle state)
- **THEN** it is absent from saved YAML and untouched during load

## Context

The material-slot-override feature (`8563081`) introduced two ways to serialize `unordered_map<string, shared_ptr<Asset>>` fields: a reflection-driven generic path in `serialization.cpp` (with map support + `load_asset_by_guid`) and manual YAML blocks in `scene.cpp`. Two separate asset-ref-detection mechanisms existed: `FieldInfo::is_asset_ref` for top-level fields and `ContainerInfo::is_element_asset_ref` for container elements.

## Goals / Non-Goals

**Goals:**
- Scene save/load delegates `override_materials` to the generic reflection serializer.
- One asset-ref trait drives both field and container-element detection.

**Non-Goals:**
- Changing the YAML format or breaking existing scene files.

## Decisions

### D1. Use the reflection serializer for `override_materials`
Remove the manual YAML blocks in `scene.cpp` and call `serialize_field`/`deserialize_field` for mesh component types so container-of-asset-ref fields flow through the generic path.

### D2. Unify asset-ref detection under one trait
`is_asset_ref_field_v<T>` (in `core/reflection.h`) is the single trait; `ContainerInfo::is_element_asset_ref` is derived from it. This is the current implementation state.

## Risks / Trade-offs

- [Behavioral drift between generic and manual serializers] → Eliminated by removing the manual path entirely.

## Migration Plan

No serialization-format or asset changes; existing scene files remain valid (verified in place).

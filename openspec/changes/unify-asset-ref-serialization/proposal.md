## Why

The material-slot-override system introduced two separate code paths for serializing `unordered_map<string, shared_ptr<Asset>>` fields:

1. A reflection-driven generic path in `serialization.cpp` (map support + `load_asset_by_guid` in `ContainerInfo`)
2. Manual YAML read/write in `scene.cpp` for the same override-materials maps

This duplication means the manual path in `scene.cpp` will drift from the generic path, and any new container-of-asset-refs fields added to components will need yet another manual block. Additionally, `FieldInfo::is_asset_ref` (for top-level fields) and `ContainerInfo::is_element_asset_ref` (for container elements) are two separate mechanisms for the same concept — they should be unified.

## What Changes

- Replace the manual `override_materials` serialize/deserialize blocks in `scene.cpp` with calls to the reflection-based `serialize_field`/`deserialize_field` (which now fully support map containers with asset-ref elements)
- Unify `FieldInfo::is_asset_ref` and `ContainerInfo::is_element_asset_ref` into a single trait — either make `is_element_asset_ref` derive from the element's `FieldInfo`-equivalent, or add a shared `AssetRefTraits` helper used by both

## Capabilities

### Modified Capabilities

- `reflection-serialization`: Scene save/load for mesh components delegates `override_materials` to the generic reflection serializer instead of manual YAML handling
- `reflection-type-traits`: Asset-ref detection is unified under one mechanism rather than split across `FieldInfo` and `ContainerInfo`

## Impact

- `engine/runtime/source/scene/scene.cpp` — remove manual override_materials YAML blocks (~40 lines removed); call `serialize_field`/`deserialize_field` for mesh component types
- `engine/runtime/source/core/reflection.h` — potentially add a shared `is_asset_ref_v` helper used by both field and container paths; or make `FieldInfo` carry a `ContainerInfo` for element-level traits

## Non-goals

- Does not change the YAML format or break existing scene files
- Does not remove `asset_ref_from_guid_string` (kept as fallback for non-container asset-ref deserialization)

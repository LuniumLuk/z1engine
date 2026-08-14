## 1. Reflection-driven serialization

- [x] 1.1 Replace manual `override_materials` YAML serialize/deserialize blocks in `scene.cpp` with `serialize_field`/`deserialize_field` calls for mesh component types
- [x] 1.2 Verify scene save/load round-trips material overrides through the generic reflection path

## 2. Unified asset-ref trait

- [x] 2.1 Make `ContainerInfo::is_element_asset_ref` derive from the shared `is_asset_ref_field_v<T>` trait in `core/reflection.h`
- [x] 2.2 Ensure `FieldInfo::is_asset_ref` and container-element detection use the same mechanism

## 3. Validation

- [x] 3.1 `python dev/z1.py compile` — 0 errors
- [x] 3.2 `python dev/z1.py test` — scene serialization tests pass
- [x] 3.3 Confirm no YAML format change and existing scene files remain loadable

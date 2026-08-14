# reflection-serialization Specification

## Purpose
TBD - created by archiving change unify-asset-ref-serialization. Update Purpose after archive.
## Requirements
### Requirement: Reflection-driven scene serialization for asset containers

Scene save/load SHALL serialize mesh `override_materials` (a `unordered_map<string, shared_ptr<Asset>>`) through the generic reflection serializer (`serialize_field`/`deserialize_field`) rather than manual YAML handling in `scene.cpp`.

#### Scenario: Mesh material overrides round-trip
- **WHEN** a scene containing mesh components with `override_materials` is saved and reloaded
- **THEN** the override map is preserved with each slot name mapped to the correct material asset

#### Scenario: No manual serialization block
- **WHEN** `scene.cpp` is inspected
- **THEN** it contains no manual `override_materials` YAML read/write blocks


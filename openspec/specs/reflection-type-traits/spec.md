# reflection-type-traits Specification

## Purpose
TBD - created by archiving change unify-asset-ref-serialization. Update Purpose after archive.
## Requirements
### Requirement: Unified asset-reference trait

Asset-reference detection SHALL use a single trait (`is_asset_ref_field_v<T>`) for both top-level fields (`FieldInfo::is_asset_ref`) and container elements (`ContainerInfo::is_element_asset_ref`).

#### Scenario: Element trait derives from field trait
- **WHEN** a container-of-asset field is reflected
- **THEN** `is_element_asset_ref` is computed from the same `is_asset_ref_field_v<T>` trait as top-level asset fields

#### Scenario: No duplicated detection logic
- **WHEN** `core/reflection.h` is inspected
- **THEN** field and container-element asset-ref detection both reference the unified trait rather than separate mechanisms


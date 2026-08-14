# reflection-driven-asset-viewer Specification

## Purpose
TBD - created by archiving change reflection-driven-asset-viewer. Update Purpose after archive.
## Requirements
### Requirement: Reflection-driven asset browser

The editor SHALL provide an asset browser that opens any asset type's metadata and displays the asset's reflected fields in the inspector.

#### Scenario: Open an asset
- **WHEN** an asset is selected/opened in the browser
- **THEN** the inspector renders its reflected fields for editing

#### Scenario: Drag-drop into the scene
- **WHEN** an asset item is dragged onto the scene viewport
- **THEN** it is accepted as an `ASSET_ITEM` payload and applied to the target component

### Requirement: Save-back for writable assets

For asset types that support saving, the editor SHALL persist edited reflected fields back to the asset file using the reflection serializer.

#### Scenario: Save an edited asset
- **WHEN** the user edits an asset's reflected fields and triggers save
- **THEN** the asset file is rewritten with the updated values via the generic serialization path


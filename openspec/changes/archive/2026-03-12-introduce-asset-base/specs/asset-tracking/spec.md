## ADDED Requirements

### Requirement: Unified Asset Registry

The `AssetManager` MUST maintain a registry of all loaded assets, indexed by their unique GUID, allowing retrieval of any asset instance regardless of its concrete type.

#### Scenario: Retrieve Loaded Asset
- **WHEN** an asset of type `Texture` is loaded via `AssetManager::get<Texture>(guid)`
- **THEN** it MUST be retrievable as an `AssetBase*` pointer via a unified `get_loaded_asset(guid)` method.
- **AND** the returned pointer MUST point to the same memory location as the original `shared_ptr`.

#### Scenario: Asset Unloading
- **WHEN** `AssetManager::scan_content` is called (clearing all assets)
- **THEN** the unified asset registry MUST also be cleared.
- **AND** queries for previously loaded assets via `get_loaded_asset(guid)` MUST return null.

### Requirement: Update Asset Metadata on Move

The `AssetManager` MUST update the metadata of the in-memory asset instance when `move_asset` is called.

#### Scenario: Update Path on Move
- **WHEN** an asset is currently loaded in memory
- **AND** `AssetManager::move_asset(guid, new_path)` is called
- **THEN** the in-memory instance's `path` metadata MUST be updated to `new_path`.
- **AND** the in-memory instance's `root` metadata MUST remain correct.

#### Scenario: Update Name on Move
- **WHEN** an asset is currently loaded in memory
- **AND** `AssetManager::move_asset(guid, new_path)` is called
- **THEN** the in-memory instance's `name()` method MUST reflect the new filename.

### Requirement: System Stability

The changes MUST ensure the project remains buildable and the editor runtime is stable.

#### Scenario: Build Verification
- **WHEN** the project is built using `dev\build_vs2026.bat`
- **THEN** the build MUST succeed without compilation errors.

#### Scenario: Runtime Verification
- **WHEN** the editor is launched with `engine\bin\Debug\game.exe --frames=10`
- **THEN** it MUST run for 10 frames and exit gracefully without crashing.

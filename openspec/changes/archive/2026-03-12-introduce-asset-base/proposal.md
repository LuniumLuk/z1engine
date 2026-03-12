## Why

The current asset system cannot update the metadata of loaded assets when they are moved or renamed because `AssetManager` does not maintain a unified registry of active instances. The `Asset<T>` template design (CRTP) lacks a common base class, making it impossible to iterate over all loaded assets or store them in a single collection. This results in stale runtime data when assets are modified on disk.

## What Changes

- **Introduce `AssetBase`**: Create a non-template base class for all assets that holds `AssetMeta` and `Guid`.
- **Inheritance Update**: Modify `Asset<T>` to inherit from `AssetBase`.
- **Unified Registry**: Update `AssetManager` to maintain a registry of all loaded assets (e.g. `std::unordered_map<Guid, AssetBase*>`) to track instances.
- **Update Logic**: Implement the TODO in `AssetManager::move_asset` to use this registry to find and update the metadata of loaded instances.

## Capabilities

### New Capabilities
- `asset-tracking`: Core capability to track and retrieve any loaded asset by its GUID regardless of its specific type.

### Modified Capabilities
<!-- None -->

## Impact

- **Core Engine**: `Asset` and `AssetManager` classes.
- **Asset Types**: All classes deriving from `Asset<T>` will indirectly inherit from `AssetBase`.
- **Editor**: Move/Rename operations will now correctly update runtime instances.

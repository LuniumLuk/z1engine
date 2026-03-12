## Context

The current `Asset` implementation uses `struct API Asset<Derived>` (CRTP). `AssetManager` stores assets in `m_storages` which is `std::unordered_map<size_t, std::any>`. The `std::any` erases type information, preventing iteration over all loaded assets. `AssetManager::move_asset` needs to update metadata of any loaded asset given its GUID, which is currently impossible without type information.

## Goals / Non-Goals

**Goals:**
- Enable `AssetManager` to access any loaded asset by GUID via a common interface.
- Allow `AssetManager` to update `AssetMeta` of loaded assets during move/rename operations.
- Maintain existing API for `Asset<T>` so specific asset types don't need changes.

**Non-Goals:**
- Implementing a full object reflection system.
- Changing the fundamental ownership model (shared_ptr) of assets.

## Decisions

### 1. Introduce `AssetBase`
We will introduce a non-template base class:
```cpp
struct API AssetBase {
    virtual ~AssetBase() = default;
    AssetMeta m_meta;
    mutable bool m_is_dirty = false;
    mutable bool m_is_saved = false;
};
```
The `Asset<T>` template will inherit from `AssetBase` instead of defining these members itself.

### 2. Secondary Registry in `AssetManager`
`AssetManager` will maintain a secondary index:
```cpp
std::unordered_map<Guid, AssetBase*> m_loaded_assets;
```
- **Updates**: Whenever an asset is loaded via `get<T>` or created, it will be added to this map.
- **Synchronization**: Since `m_storages` holds the strong references and effectively owns the assets (as a cache), this secondary map will hold raw pointers. The map entries must be managed strictly in tandem with `m_storages`.

### 3. Update `move_asset` logic
The `move_asset` function will query `m_loaded_assets` using the GUID. If found, it will update `m_meta.path` and `m_meta.extra` directly on the instance.

## Risks / Trade-offs

- **Risk: Dangling Pointers**: If an asset is unloaded or destroyed but remains in `m_loaded_assets`, we get a crash.
  - *Mitigation*: `AssetManager` does not currently support partial unloading. It clears everything in `scan_content`. We must ensure `m_loaded_assets` is cleared whenever `m_storages` is cleared.

- **Trade-off: Virtual Destructor**: Adding `virtual ~AssetBase` adds a vtable pointer to every asset.
  - *Impact*: Negligible overhead for asset types (which are usually heavy resources anyway).

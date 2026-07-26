## 1. Core Asset Structure

- [x] 1.1 Define `AssetBase` class in `engine/runtime/source/asset/asset.h`.
- [x] 1.2 Move `AssetMeta` and state flags (`m_is_dirty`, `m_is_saved`) to `AssetBase`.
- [x] 1.3 Update `Asset<T>` to inherit from `AssetBase`.

## 2. AssetManager Registry

- [x] 2.1 Add `std::unordered_map<Guid, AssetBase*> m_loaded_assets` to `AssetManager` class in `engine/runtime/source/asset/asset_manager.h`.
- [x] 2.2 Update `AssetManager::get<T>` template to register loaded assets into `m_loaded_assets`.
- [x] 2.3 Update `AssetManager::scan_content` to clear `m_loaded_assets`.
- [x] 2.4 Add `AssetBase* get_loaded_asset(Guid const& guid)` method to `AssetManager`.

## 3. Metadata Update Logic

- [x] 3.1 Update `AssetManager::move_asset` in `engine/runtime/source/asset/asset_manager.cpp` to find the loaded instance using `m_loaded_assets`.
- [x] 3.2 Implement the logic to update the instance's metadata (path, name) when moved.

## 4. Verification

- [x] 4.1 Build the project using `dev\build_vs2026.bat` to ensure no compilation errors.
- [x] 4.2 Verify runtime stability by running `engine\bin\Debug\game.exe --frames=10` and ensuring it runs for 10 frames and exits without crashing.

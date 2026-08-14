## 1. Asset browser

- [x] 1.1 Add an editor asset browser with open/select callbacks (`m_browser->m_on_asset_opened`)
- [x] 1.2 Support dragging asset items (`ASSET_ITEM` payload) onto scene components

## 2. Reflection-driven inspector

- [x] 2.1 Implement `show_asset_info()` rendering an asset's reflected fields
- [x] 2.2 Wire asset save for writable asset types through the reflection serializer

## 3. Validation

- [x] 3.1 `python dev/z1.py compile` — 0 errors
- [x] 3.2 Editor smoke test passes

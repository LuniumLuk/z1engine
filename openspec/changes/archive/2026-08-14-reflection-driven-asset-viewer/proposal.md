## Why

The editor needs a general asset-viewing workflow that is reflection-driven rather than hand-written per asset type, so new asset types get editor support without bespoke panels.

## What Changes

- An editor asset browser (`m_browser`) with open/select callbacks and drag-drop of asset items
- Reflection-driven `show_asset_info()` that renders editable fields based on reflected asset metadata
- Asset save support for asset types that support it (reusing the reflection serializer)

## Capabilities

### New Capabilities

- `reflection-driven-asset-viewer`: The editor exposes a reflection-driven asset browser and inspector that renders and edits any asset type's reflected fields, including save-back for writable asset types.

## Impact

- `engine/editor/source/editor_layer.cpp/.h` (asset browser, `show_asset_info`, save), asset reflection plumbing
- Committed under the reflection-system upgrade (`c3962da`)

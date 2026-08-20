# Asset System
> Summary: Asset types, binary format, bakery pipeline, and import infrastructure
> Scope: engine/runtime/source/asset/, engine/bakery/

## Core Types

| Type | Header | Role |
|------|--------|------|
| `AssetBase` | `asset/asset.h` | Base class for all assets (state, dirty flags) |
| `Asset<T>` | `asset/asset.h` | Templated asset with typed data |
| `AssetManager` | `asset/asset_manager.h` | Loading, caching, scanning by GUID |
| `BinaryFile` | `asset/binary_file.h` | Engine binary format read/write |

## Asset Types

| Asset | Header | Extension |
|-------|--------|-----------|
| `Texture` | `asset/texture.h` | Baked binary from `.png`/`.tga` |
| `Mesh` | `asset/mesh.h` | Baked binary from `.gltf`/`.obj` |
| `Material` | `asset/material.h` | Material properties |
| `ScriptAsset` | `asset/script_asset.h` | Python script (`.py`) |
| `Scene` | `scene/scene.h` | Scene graph (`.scene` YAML) |

## Asset Identification

- Assets identified by **GUID** (UUID), not file path
- `AssetManager::get<T>(guid)` loads and caches
- `AssetManager::scan_content()` discovers all assets in content directories

## Asset Load Interface (CRTP)

- `Asset<T>::load(guid)` resolves `AssetMeta` + `Filepath` from `AssetManager`, then calls `T::load(guid, meta, file)`
- Asset types declare `create`/`load`/`save` between `// --- begin asset interface ---` / `// --- end asset interface ---` markers
- Call sites use `Asset<T>::load(guid)` (uncached) or `AssetManager::get<T>(guid)` (cached); the 1-arg `T::load(guid)` is gone
- `asset/asset.h` includes `asset_manager.h` after `AssetMeta`/`AssetBase`; the `load` wrapper is defined out-of-line to avoid a circular include

## Importers (`asset/importer/`)

| Importer | Source Format | Role |
|----------|--------------|------|
| `GltfImporter` | `.gltf`/`.glb` | 3D model import |
| `ObjImporter` | `.obj` | Wavefront OBJ import |
| `TextureImporter` | `.png`/`.tga`/`.exr` | Texture import + compression |
| `Importer` (base) | -- | Importer interface |

## Bakery Pipeline

- Standalone CLI tool at `engine/bakery/`
- Scans source assets, converts to engine-optimized binary format
- Key source files:
  - `bakery/source/baker/image.cpp` -- image baking
  - Uses tinyobjloader, tinyexr, stb (in `bakery/source/3rdparty/`)

## Binary Format

- Custom binary format via `BinaryFile` class
- Optimized for fast loading (memory-mapped compatible)
- Baked assets stored alongside source in content directories

## Material Editor (`engine/editor/source/material_editor.h`)

- `MaterialEditor` window opens on double-click of material/material instance in the content browser; owns a preview `Scene` (procedural UV sphere + two directional lights + camera) rendered by a dedicated `RendererForward`/`RendererDeferred` instance into a fixed 512x512 framebuffer. Preview image lives in an `ImGui::BeginChild` (main-viewport pattern) so orbit drags don't move the window.
- Editing: `Material` flags (alpha mode, cull, depth test/write) + `m_variables`; `MaterialInstance` `m_override_variables` with valid-flag checkboxes; Sampler2D slots accept `ASSET_ITEM` drag-drop of texture2d assets. Save button persists.
- `Material::save()` writes a `variables:` YAML sequence (`name`/`type`/`value`, same shape as `MaterialInstance` `overrides`); `Material::load()` reads it back (guarded by `IsSequence()` for backward compat). Previously variables were never persisted (load only read `flags`/`shader`; reflection save emitted nulls).
- Round-trip covered by `engine/test/test_material_roundtrip.cpp` (auto-discovered by `create_test`).
- `MaterialFlags::get_blend`/BlendMask is dead — pipeline blend derives from `AlphaMode` only; not exposed in the editor.

-> see [architecture.md]
-> see [render-pipeline.md]

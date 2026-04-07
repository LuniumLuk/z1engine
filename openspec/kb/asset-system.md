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

-> see [architecture.md]
-> see [render-pipeline.md]

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

## Asset Identification (deterministic guids)

- Assets are identified by a 128-bit `Guid` (`core/guid.h`) derived from **root + path** — never random, never stored text.
- `AssetManager::get<T>(guid)` loads and caches; `AssetManager::scan_content()` discovers assets in content directories.
- `Guid` is `uint32_t m_data[4]` (word 0 = the first 8 hex digits); text form is 32 lowercase hex digits, no dashes; all-zero = invalid.
- Derivation: two domain-separated FNV-1a-64 passes (`"z1a:" + key`, `"z1b:" + key`) form the 128-bit hash.
  - Engine root (`root == "engine"`): key is the path alone and word 0 is forced to 0 → ids start with `00000000`.
  - Any other root: key is `root + "/" + path`; if word 0 would be zero it is set to `0x80000000` so the id spaces stay disjoint.
- `scan_content()` **derives** `meta.guid` from root + path and ignores the stored yaml value (a stale or missing guid is informational only).
- References resolve by legacy path string (`mesh/SM_Cube`, `shader/pbr`) or by 32-hex id (`AssetManager::resolve_guid`); saving always writes hex.
- `.bin` assets embed a yaml copy: mesh `meta.guid` is ignored by loaders, but `primitives[].material` ids are **live** references.
- Tooling (all import `utils/z1_guid.py`, which mirrors the C++ hash bit-for-bit):
  - `utils/migrate_guids.py` — one-shot migration of yaml, `.bin` embedded yaml and script/shader references (idempotent).
  - `utils/gen_content.py` — regenerates `engine/content` meshes/textures with correct engine ids.
  - `engine/tool/assetkit/assetkit_core.py` — validates derived ids, stale ids and references (yaml + `.bin`).
- Test vectors shared by `engine/test/test_guid.cpp` and `utils/z1_guid.py --self-test`, e.g. engine `mesh/SM_Cube` = `000000009a9d0329a8631e5aa760403e`.

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

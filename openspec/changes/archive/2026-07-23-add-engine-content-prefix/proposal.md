## Why

Engine-builtin assets currently share the same GUID/path namespace as user-created content. A user who creates `material/M_pbr.yaml` in `content/` would silently shadow the engine's PBR material. The engine only supports two hardcoded roots (`content/` and `engine/content/`), with no way to add plugin or module roots. A generalized multi-root system with `$prefix`-based routing solves both the collision problem and enables arbitrary content roots.

## What Changes

- **BREAKING**: `FileSystem::s_content_root` / `s_engine_root` replaced by a configurable ordered root registry (`std::vector<RootConfig>`). Default roots: `"" → ./content` (priority 0), `"engine" → ./engine/content` (priority 10). Users can add arbitrary named roots.
- **BREAKING**: `AssetManager::scan_content()` iterates over all registered roots. Asset uniqueness is guaranteed by `build_internal_path` keys in `m_path_to_guid_mapping` (e.g., `$engine/material/M_pbr`), not by GUID mutation. YAML `guid` fields pass through unchanged.
- `$prefix` syntax for root-qualified lookups: `$engine/shader/pbr` searches ONLY the engine root. Unprefixed paths (`shader/pbr`) search roots in priority order (default content first, then engine, then plugins).
- `build_internal_path(meta)` generates internal path keys (`"$rootname/path"` for named roots, `"path"` for default). `resolve_guid(str)` provides backward-compatible YAML GUID resolution with priority fallback.
- Asset YAML files on disk are **unchanged** — the `guid` field is preserved verbatim as the runtime GUID.
- Hardcoded engine paths in C++ updated to use the path-based `get<T>("$engine/...")` overload instead of `Guid::make("$engine/...")`.

## Capabilities

### New Capabilities

- `multi-root-asset-system`: A configurable ordered list of named content roots. Assets are internally keyed by `rootname/path` for global uniqueness. The `$rootname/` prefix in lookup paths restricts search to a specific root; unprefixed paths fall back through roots in priority order.

### Modified Capabilities

<!-- No existing specs are modified by this change. -->

## Impact

- **`engine/runtime/source/core/io.h` / `io.cpp`**: Replace `s_content_root`, `s_engine_root` with `RootConfig` struct and `static std::vector<RootConfig> s_roots`
- **`engine/runtime/source/asset/asset_manager.h` / `.cpp`**: Multi-root scan loop, `build_internal_path()`, root-aware `get_guid_from_path()`, `resolve_guid()`, `get_root_for_meta()` generalized
- **`engine/runtime/source/core/guid.h`**: **No changes** — `Guid::make()` is unchanged; `$` is preserved in GUID strings
- **`engine/runtime/source/render/renderer/`**: 4 renderer files — hardcoded shader/material paths updated to `$engine/...`
- **`engine/editor/source/picking_system.cpp`**: 2 hardcoded shader paths updated
- **`engine/runtime/source/asset/importer/gltf_importer.cpp`**: 2 hardcoded material paths updated
- **`engine/test/test_scene_serialize.cpp`**: Hardcoded engine-mesh paths updated to `$engine/mesh/...`
- **`engine/content/` YAML files**: **No changes** — `guid` and `path` stay unprefixed
- **User content unaffected**: assets under `content/` keep their unprefixed names; unprefixed lookups continue to find them first

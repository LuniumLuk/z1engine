## 1. Root Registry — Replace hardcoded roots with configurable list

- [x] 1.1 Add `struct RootConfig { std::string name; Filepath path; int priority; };` to `engine/runtime/source/core/io.h`.
- [x] 1.2 Replace `FileSystem::s_content_root` and `FileSystem::s_engine_root` with `static std::vector<RootConfig> s_roots` in `io.h`, default-initialized in `io.cpp` with `{"", "content", 0}` and `{"engine", "engine/content", 10}`.
- [x] 1.3 Add `FileSystem::get_root_path(std::string const& name)` helper that looks up a root's filesystem path by name.
- [x] 1.4 Add `FileSystem::get_roots_ordered()` that returns roots sorted by priority (ascending).
- [x] 1.5 Update all existing references to `FileSystem::s_content_root` and `FileSystem::s_engine_root` across the codebase to use `get_root_path()` or equivalent. Search for: `s_content_root`, `s_engine_root`.

## 2. Internal key format — `$` permanently in keys, Guid::make unchanged

- [x] 2.1 `Guid::make()` is unchanged (no `$` stripping). The `$` is a permanent part of internal keys, not stripped at the Guid boundary.
- [x] 2.2 `build_internal_path(meta)` generates `"$rootname/path"` for named roots, `"path"` for default root. This is the single source of truth for internal key format.
- [x] 2.3 `resolve_guid(str)` tries `get_guid_from_path` first (with priority fallback), then falls back to `Guid::make`. Used for all YAML-internal GUID resolution.

## 3. Asset Manager — Multi-root scan and root-aware GUID generation

- [x] 3.1 Rewrite `AssetManager::scan_content()` to iterate over `FileSystem::s_roots` instead of the two hardcoded root blocks. For each root, scan `.yaml` and `.glsl`/`.py` files as before.
- [x] 3.2 YAML `guid` field passes through to `AssetMeta::guid` unchanged — no GUID override. Uniqueness is guaranteed by `build_internal_path` keys in `m_path_to_guid_mapping`, not by GUID mutation. All C++ code uses path-based `get<T>(\"$engine/...\")` overloads instead of `Guid::make(\"$engine/...\")`.
- [x] 3.3 Change `register_asset()` to use `build_internal_path(meta)` as the key for `m_path_to_guid_mapping` (instead of plain `meta.path`), to enable root-qualified lookups.
- [x] 3.4 Update `get_root_for_meta()` to use `FileSystem::get_root_path(meta.root)` instead of the old `if (meta.root == "engine")` branching.

## 4. Asset Manager — $prefix-aware path lookup

- [x] 4.1 Update `get_guid_from_path(Filepath const& path)`: detect leading `$`. If present, use full `$rootname/path` as direct internal key lookup. If not present, iterate roots in priority order building `"$rootname/path"` (or just `path` for default root) until found.
- [x] 4.2 Update `has_path(Filepath const& path)` — delegates to `get_guid_from_path` for consistent `$prefix`-aware logic.
- [x] 4.3 Update `find_references(Guid const& guid)` to use `get_root_for_meta` instead of direct `root / meta.path` for filesystem path construction.

## 5. Filesystem path resolution — Use root path + relative meta.path

- [x] 5.1 Verified all places that construct filesystem paths from `meta.path` use `get_root_for_meta(meta) / meta.path`. Since `meta.path` stays unprefixed (relative within its root), no stripping is needed.

## 6. C++ Code — Update hardcoded engine asset references to use $engine/ prefix

- [x] 6.1 Update `engine/editor/source/picking_system.cpp`: `"shader/picking"` → `"$engine/shader/picking"`, `"shader/picking_sprite"` → `"$engine/shader/picking_sprite"`
- [x] 6.2 Update `engine/runtime/source/render/renderer/particle_renderer.cpp`: `"shader/particle"` → `"$engine/shader/particle"`, `"shader/particle_shadow"` → `"$engine/shader/particle_shadow"`
- [x] 6.3 Update `engine/runtime/source/render/renderer/render_shared.cpp`: all 4 shader references (`postprocessing`, `taa`, `bloom_downsample`, `bloom_upsample`) → `$engine/shader/...`
- [x] 6.4 Update `engine/runtime/source/render/renderer/renderer_2d.cpp`: `"shader/sprite_2d_batched"` → `"$engine/shader/sprite_2d_batched"`
- [x] 6.5 Update `engine/runtime/source/render/renderer/renderer_deferred.cpp`: `Guid::make("material/MI_phone")` → `Guid::make("$engine/material/MI_phone")`, `"shader/deferred_lighting"` → `"$engine/shader/deferred_lighting"`, `"shader/deferred_skybox"` → `"$engine/shader/deferred_skybox"`
- [x] 6.6 Update `engine/runtime/source/render/renderer/renderer_forward.cpp`: `Guid::make("material/MI_phone")` → `Guid::make("$engine/material/MI_phone")`, `"shader/skybox"` → `"$engine/shader/skybox"`
- [x] 6.7 Update `engine/runtime/source/asset/importer/gltf_importer.cpp`: `"material/M_pbr"` → `"$engine/material/M_pbr"`, `"material/M_pbr_sg"` → `"$engine/material/M_pbr_sg"`
- [x] 6.8 Update `engine/test/test_scene_serialize.cpp`: `"mesh/SM_Cube"` → `"$engine/mesh/SM_Cube"`, `"mesh/SM_Sphere"` → `"$engine/mesh/SM_Sphere"`, `"mesh/SM_Cone"` → `"$engine/mesh/SM_Cone"`, `std::string("mesh/")` → `std::string("$engine/mesh/")`

## 7. Verify shader validator is unaffected

- [x] 7.1 Confirm `engine/tool/shader_validator/main.cpp` uses filesystem path `"engine/content/shader/"`, not asset GUIDs. No change needed.
- [x] 7.2 Run `python dev/z1.py validate-shaders` to confirm all shaders still compile.

## 8. Build and test

- [x] 8.1 Run `python dev/z1.py generate` to regenerate project files.
- [x] 8.2 Run `python dev/z1.py compile` — all 12 projects succeeded, 0 errors, 0 warnings.
- [x] 8.3 Run `python dev/z1.py test` — all 8 tests pass.
- [x] 8.4 Run `python dev/z1.py dcv` for full develop-compile-verify cycle.

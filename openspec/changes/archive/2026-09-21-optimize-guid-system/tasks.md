## 1. Guid core

- [x] 1.1 Rewrite `core/guid.h`: 128-bit value type (`uint32_t m_data[4]`), `to_string`/`from_string`,
  `from_path`/`from_root_and_path` derivation (two FNV-1a-64 passes, engine zero prefix, foreign flip rule),
  `is_valid`, equality and `std::hash`; delete `Guid::generate()`
- [x] 1.2 Add the Python mirror `utils/z1_guid.py` and record shared test vectors for C++/Python parity
- [x] 1.3 Update `util/yaml.h` `convert<Guid>` (lenient hex decode, hex encode) and `scene/serialization.cpp`
  guid serialization plus plain-`Guid` field deserialization through `resolve_guid`
- [x] 1.4 Make `AssetManager::scan_content()` derive ids for yaml assets, scripts and shaders; keep duplicate-id
  reporting; adjust `register_asset`/`resolve_guid`/`get_guid_from_path`

## 2. Call sites

- [x] 2.1 Derive ids in importers (`TextureImporter`, `ObjImporter`, `GltfImporter`) and in
  `Material::create`, `MaterialInstance::create`, `Mesh::Storage::import`
- [x] 2.2 Update editor call sites (`browser.h` copy guid, `editor_layer.cpp` last scene, `type_field.cpp`,
  `material_editor.h`) to the value type
- [x] 2.3 Register `py::class_<Guid>` in `py_engine.cpp` so generated bindings keep compiling and working
- [x] 2.4 Compile the whole solution with 0 errors after `python dev/z1.py generate`

## 3. Tests

- [x] 3.1 Add `engine/test/test_guid.cpp` (vectors, engine prefix, flip rule, parse/format round-trip, invalid
  input, scan override of a stale guid, reference resolution by path and by id)
- [x] 3.2 Run `python dev/z1.py test` and fix regressions (scene serialize, material roundtrip, import)

## 4. Content migration

- [x] 4.1 Add `utils/migrate_guids.py` (pass 1 old→new map, pass 2 rewrite guids + uuid references, idempotent)
- [x] 4.2 Run it over `engine/content/` and `content/`; verify the second run is a no-op and references resolve
  (also migrates the yaml embedded in `.bin` assets, including primitive material ids via sidecar pairing)

## 5. Content generator

- [x] 5.1 Add `utils/gen_content.py`: optimized indexed meshes (cube, cone, cylinder, sphere, new plane; Suzanne
  untouched) and textures (black, magenta, normal, white, new checker, grid, noise) written with the new guid form
- [x] 5.2 Generate content and verify loading end-to-end (`test`, `smoke`)

## 6. Docs and final verification

- [x] 6.1 Update `engine/tool/assetkit/assetkit_core.py` validation to the derived-id rules (adds `.bin` scanning)
- [x] 6.2 Update `openspec/kb/asset-system.md` (and index if needed) with the guid scheme
- [x] 6.3 Run `python dev/z1.py format --dry-run` (clean apart from a pre-existing space-indented script) and
  `python dev/z1.py dcv --auto`: generate/compile/format/test pass (Hybrid 12/12, Debug 14/14); the benchmark step
  fails on `missing metric source: profile-run.json` — a pre-existing config mismatch (suites run
  `engine/bin/Debug/game.exe` while `ENABLE_PROBING` is opt-in for Hybrid only), unrelated to this change. Two
  pre-existing `datetime.UTC` crashes on the CLI's Python 3.10 (benchmark report, DCV report) were fixed to
  `datetime.timezone.utc` so the loop runs to completion.

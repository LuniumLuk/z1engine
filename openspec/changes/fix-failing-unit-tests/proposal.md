## Why

Three unit tests (`test_import`, `test_render_graph`, `test_scene_serialize`) fail, blocking the `test` and `dcv` validation gates. Two fail because test executables ship without the Python standard library, and `test_scene_serialize` additionally drives the script system through the wrong scene update method so its Python script assertion never runs. These are environment/packaging + test-logic defects, not engine feature regressions.

## What Changes

- **Test runtime packaging** (`premake5.lua`): `create_test()` copies `python314.zip` next to test executables in addition to `python314.dll`, matching what `engine/game/premake5.lua` already does for the game. This fixes `test_import` and `test_render_graph` (and the interpreter-init crash in `test_scene_serialize`).
- **`test_scene_serialize.cpp`**: the Python script section calls `scene->on_fixed_update()` (where `ScriptSystem::update` runs) instead of `scene->on_update()`, so the `sandbox_tests.test_mover` script is attached, started, and updated, and the "script moved entity" assertion verifies real behavior.
- Scope is limited to the failing tests only; no other tests are modified.

## Capabilities

### New Capabilities

- `test-runtime-packaging`: The build deploys the embedded Python runtime (`python314.dll` + `python314.zip`) beside every test executable so `python dev/z1.py test` runs from a clean build without manual file copying.
- `scene-serialize-python-script-test`: The scene-serialization test drives scene scripts through the fixed-update path (`Scene::on_fixed_update`) and verifies the attached Python script actually mutates the entity transform.

## Impact

- `premake5.lua` — `create_test()` postbuild commands (add `python314.zip` copy)
- `engine/test/test_scene_serialize.cpp` — Python script test calls `on_fixed_update()`
- No engine runtime code, shaders, or other tests change

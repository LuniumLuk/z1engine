## 1. Test runtime packaging (premake5.lua)

- [x] 1.1 Add a `python314.zip` copy to `create_test()`'s Windows `postbuildcommands` in `premake5.lua`, mirroring `engine/game/premake5.lua` (same `if not exist` guard as the dll)
- [x] 1.2 Regenerate projects: `python dev/z1.py generate`

## 2. test_scene_serialize fix

- [x] 2.1 Change the Python script test in `engine/test/test_scene_serialize.cpp` to call `scene->on_fixed_update()` instead of `scene->on_update(0.1f)` (keep the comment accurate)

## 3. Validation

- [x] 3.1 `python dev/z1.py compile` — 0 errors
- [x] 3.2 Remove any manually copied `python314.zip` from `engine/bin/test/Debug/` and verify `python dev/z1.py test` repopulates it via postbuild
- [x] 3.3 `python dev/z1.py test` — all 8 tests pass (including `test_import`, `test_render_graph`, `test_scene_serialize`)
- [x] 3.4 Confirm no other tests were modified

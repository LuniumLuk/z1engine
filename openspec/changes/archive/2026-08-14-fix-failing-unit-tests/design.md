## Context

`create_test()` in `premake5.lua` generates one ConsoleApp per `engine/test/*.cpp`, targeting `engine/bin/test/<Config>/`. Its Windows postbuild copies only `python314.dll`; the game project (`engine/game/premake5.lua`) copies both `python314.dll` and `python314.zip`. The tests link and embed Python 3.14 via `PythonLayer`, which initializes the interpreter at startup.

The interpreter-init failure is reproducible: with `python314.zip` absent from the test target dir, `Py_InitializeFromConfig` fails with *"Failed to import encodings module"* (the stdlib zip is resolved from the executable directory in this embedded configuration), tripping `CORE_ASSERT` and exiting with `STATUS_BREAKPOINT` (`0x80000003`). Copying the zip beside the test exe makes `test_import` and `test_render_graph` pass.

`test_scene_serialize` additionally fails its Python script assertion. The test calls `scene->on_update(0.1f)`, but `Scene::on_fixed_update()` is where `ScriptSystem::update` runs (moved there in commit `305688a`). `on_update` only snapshots previous transforms and runs post-processing, so the `sandbox_tests.test_mover` script is never attached/updated and the entity transform never changes.

## Goals / Non-Goals

**Goals:**
- `python dev/z1.py test` passes all 8 tests from a clean build (no manual file copying).
- `test_scene_serialize` genuinely verifies its Python script behavior.
- Keep the change scoped to the 3 failing tests.

**Non-Goals:**
- Fixing the pre-existing `validate-shaders` wrapper heuristic (separate tooling issue).
- Adding/removing other tests or changing the test framework.
- Changing the embedded-Python home-resolution logic in `python_layer.cpp` (works once the zip is present; out of scope).

## Decisions

### D1. Copy `python314.zip` in `create_test()` postbuild
Mirror `engine/game/premake5.lua`: add `{COPYFILE} .../python314.zip` to the test postbuild command, guarded by the same `if not exist` pattern as the dll so incremental builds are cheap.

- *Alternative considered:* relying on `get_embedded_python_home()`'s `cwd/engine/3rdparty/python314/python314.zip` fallback. Rejected: empirically the exe-dir zip is what the embedded interpreter resolves (verified by remove/re-add), and the game already ships the zip beside the exe — consistency wins.
- *Alternative considered:* bundling a shared runtime dir. Rejected: the existing per-target-dir layout already works for the game; mirror it.

### D2. `test_scene_serialize` drives scripts via `on_fixed_update`
Change the Python script test from `scene->on_update(0.1f)` to repeated `scene->on_fixed_update()` calls. `ScriptSystem::update` lazily attaches (`attach_func` → import module, instantiate, bind `entity`), calls `on_start`, then `on_update(delta)` with `Timer::fixed_update_delta` (0.016) per step; the script's `on_update` adds `delta_time` to `transform.location.x`. A single fixed step would only move x by 0.016 (below the `x > 0.05` assertion), so the test derives the step count from the delta (`int(min_movement / Timer::fixed_update_delta) + 1` → 4 steps at 0.016), accumulating `x ≈ 0.064`, which passes and stays correct if `fixed_update_delta` ever changes.

- *Alternative considered:* routing `ScriptSystem::update` back into `on_update`. Rejected: scripts intentionally run on the fixed timestep (physics/animations) — the test was stale, not the engine.

## Risks / Trade-offs

- [Copied zip grows the test output dir by ~4 MB] → Accepted: matches the game layout; same file, already shipped.
- [`on_fixed_update` also runs PhysicsSystem on Windows in the test] → The test scene has no physics components (no `PhysicsComponent`/colliders) and a short single call; harmless. The test previously only exercised `on_update`, so this adds animation/particle/physics no-ops — negligible.
- [Test still depends on `content/` write access for the sandbox script] → Unchanged existing behavior; the test cleans up after itself (`cleanup_scene()`).

## Migration Plan

No serialization or API changes. Regenerate projects (`python dev/z1.py generate`) so the premake postbuild change takes effect, rebuild, and run `python dev/z1.py test`. Rollback: revert the two file changes; no asset migrations required.

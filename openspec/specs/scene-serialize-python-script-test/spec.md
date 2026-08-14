# scene-serialize-python-script-test Specification

## Purpose
TBD - created by archiving change fix-failing-unit-tests. Update Purpose after archive.
## Requirements
### Requirement: Scene-serialize test drives scripts through fixed update

The Python script test in `test_scene_serialize.cpp` SHALL invoke `Scene::on_fixed_update()` (where `ScriptSystem::update` runs) so the attached `sandbox_tests.test_mover` script is attached, started, and updated during the test.

#### Scenario: Python script moves the entity
- **WHEN** the test attaches `TestMover` to an entity and calls `scene->on_fixed_update()` several times (each fixed step advances the script by `Timer::fixed_update_delta` = 0.016s, accumulating past the 0.05 assertion)
- **THEN** the entity's transform X position increases and the test reports `[SUCCESS] Python Script moved entity!` and exits 0

#### Scenario: Test passes as part of the suite
- **WHEN** `python dev/z1.py test` runs the full suite with the Python runtime deployed beside the test executables
- **THEN** `test_scene_serialize` passes with exit code 0 and all 8 tests report `[OK]`


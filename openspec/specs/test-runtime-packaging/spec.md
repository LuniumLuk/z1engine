# test-runtime-packaging Specification

## Purpose
TBD - created by archiving change fix-failing-unit-tests. Update Purpose after archive.
## Requirements
### Requirement: Test executables receive the Python runtime

Every test executable produced by `create_test()` in `premake5.lua` SHALL have both `python314.dll` and `python314.zip` copied next to it by the build's postbuild step, mirroring the game executable's deployment.

#### Scenario: Clean build deploys both Python files
- **WHEN** the solution is generated and compiled with `python dev/z1.py generate` then `python dev/z1.py compile`
- **THEN** `engine/bin/test/<Config>/` contains both `python314.dll` and `python314.zip`

#### Scenario: Tests run without manual setup
- **WHEN** `python dev/z1.py test` runs against a fresh build output directory
- **THEN** `test_import` and `test_render_graph` pass with exit code 0 and no Python interpreter-init error

#### Scenario: Incremental builds keep the files
- **WHEN** the solution is recompiled without changes to the test projects
- **THEN** `python314.dll` and `python314.zip` remain present beside the test executables


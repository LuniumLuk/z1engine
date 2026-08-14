# Testing
> Summary: Test infrastructure, how to add and run tests in z1engine
> Scope: engine/test/, engine/bin/test/

## Test Framework

- **No external test framework** -- tests are standalone `main()` programs
- Exit code 0 = pass, non-zero = fail
- Each test is a single `.cpp` file producing one executable

## Existing Tests

| Test | File | Covers |
|------|------|--------|
| `test_bakery` | `engine/test/test_bakery.cpp` | Asset baking pipeline |
| `test_binary_file` | `engine/test/test_binary_file.cpp` | Binary file read/write |
| `test_import` | `engine/test/test_import.cpp` | Asset importers |
| `test_pybind11` | `engine/test/test_pybind11.cpp` | Python binding verification |
| `test_render_graph` | `engine/test/test_render_graph.cpp` | Render graph construction |
| `test_scene_serialize` | `engine/test/test_scene_serialize.cpp` | Scene save/load |
| `test_transform` | `engine/test/test_transform.cpp` | Transform math |

## Running Tests

```cmd
python dev/z1.py test
python dev/z1.py test --filter "test_render*"
python dev/z1.py test --config Release
```

## Output Location

- Debug: `engine/bin/test/Debug/test_*.exe`
- Release: `engine/bin/test/Release/test_*.exe`

## Test Runtime (Python)

- Test executables that touch Python need `python314.dll` AND `python314.zip` (stdlib) beside them — `create_test()`'s postbuild in `premake5.lua` copies both (guarded by `if not exist`), mirroring `engine/game/premake5.lua`.
- Missing `python314.zip` → `Py_InitializeFromConfig` fails with "Failed to import encodings module" (embedded interpreter resolves the stdlib zip from the exe dir) → `CORE_ASSERT` exit `0x80000003`. See archived change `2026-08-14-fix-failing-unit-tests`.

## Adding a New Test

1. Create `engine/test/test_<name>.cpp` with a `main()` function
2. Auto-discovered by `create_test()` in root `premake5.lua` (globs `engine/test/**.cpp`)
3. Run `python dev/z1.py generate` to regenerate VS projects
4. Build with `python dev/z1.py compile`
5. Run with `python dev/z1.py test --filter "test_<name>*"`

## Test Pattern

```cpp
#include "z1engine.h"  // or relevant headers

int main() {
    // Setup
    // Execute
    // Verify (return non-zero on failure)
    return 0;
}
```

## DCV Integration

- Tests run as step 5 of the DCV loop
- Triggered when `engine/runtime/` or `engine/bakery/` code changes
- `python dev/z1.py dcv --auto` auto-detects when tests are needed

## Benchmark Gate

- Benchmark step runs after tests in DCV
- Mapped suites selected from `dev/validation/suite-map.json`
- Suite definitions in `dev/benchmark/suites/*.json`
- Baselines in `dev/benchmark/baselines/<config>/*.baseline.json`
- Regressions over configured thresholds fail the validation gate

## Workspace Hygiene + Waiver Policy

- Validation runs must not persist artifacts into live `content/**`
- Temporary artifacts must be confined to sandbox paths and cleaned after run
- Policy file: `dev/validation/policy.json`
- Temporary waiver handling:
    - if benchmark instability is known, use a documented waiver with owner + expiry
    - waivers are temporary and must be removed after baseline stabilization
    - test failures are never waived silently

-> see [build.md]

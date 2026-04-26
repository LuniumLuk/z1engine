## Test + Benchmark Design (Code-Aware)

### Existing Integration Points Mapped

- CLI registration: `dev/z1.py`
- Orchestration and step auto-detection: `dev/commands/dcv.py`
- Test executable discovery: `dev/commands/test.py` (`test_*.exe` in `engine/bin/test/<Config>/`)
- Profiling trace source for benchmark metrics: `engine/runtime/source/util/instrumentor.h` and profiles written by application (`profile-run.json`)

### Test Design

#### A. Dev Command Unit/Behavior Tests (Python)

1. dcv step-selection logic
- Target: `_detect_steps` in `dev/commands/dcv.py`
- Cases:
  - input contains `.lua` => `generate = true`
  - input contains `.glsl` => `validate-shaders = true`
  - runtime/bakery/test paths => `test = true`
  - unrelated docs/content only => compile/format only
- Pass criteria: step map equals expected dictionary.

2. test discovery and filtering
- Target: `_discover_tests` in `dev/commands/test.py`
- Cases:
  - empty folder => empty list
  - folder with `test_a.exe`, `test_b.exe`, `other.exe` => only `test_*.exe`
  - `--filter test_render*` selects expected subset
- Pass criteria: deterministic sorted result set.

3. structured RESULT contract
- Target: `make_result` usage in command modules
- Cases:
  - command success writes one final `RESULT: {json}` line
  - fail path includes non-zero semantic `exit_code` field
  - dcv aggregated output includes per-step statuses
- Pass criteria: schema keys present, parseable JSON, line is last line.

#### B. Engine Regression Test Selection

Use existing binaries from `engine/test/*.cpp` as scoped gates.

1. Core scene/entity changes
- Required tests: `test_transform`, `test_scene_serialize`, `test_import`
- Trigger paths:
  - `engine/runtime/source/scene/`
  - `engine/runtime/source/core/`
  - `engine/runtime/source/asset/`

2. Rendering pipeline changes
- Required tests: `test_render_graph`
- Trigger paths:
  - `engine/runtime/source/render/`
  - `engine/editor/source/`

3. Bakery/asset compression changes
- Required tests: `test_bakery`, `test_import`
- Trigger paths:
  - `engine/bakery/`
  - `asset/texture/`, `asset/mesh/`

4. Python integration changes
- Required tests: `test_pybind11`, `test_scene_serialize`
- Trigger paths:
  - `engine/runtime/source/python/`
  - `dev/commands/`

### Benchmark Design

#### Suites Defined

1. runtime-core
- File: `dev/benchmark/suites/runtime-core.json`
- Workloads:
  - `content/forest.yaml`
  - `content/new_scene.yaml`
- Metrics:
  - `frame_time_ms_p50`, `frame_time_ms_p95`
  - `scene_update_us_p50`
- Thresholds: 8% to 12% depending on metric.

2. rendering-pipeline
- File: `dev/benchmark/suites/rendering-pipeline.json`
- Workloads:
  - `content/new_scene_1.yaml`
  - `content/AlphaBlendModeTest/AlphaBlendModeTest.prefab.yaml`
- Metrics:
  - `render_frame_ms_p50`, `render_frame_ms_p95`
  - `shader_bind_us_p50`
- Thresholds: 6% to 12% depending on metric.

#### Baselines

- `dev/benchmark/baselines/debug/runtime-core.baseline.json`
- `dev/benchmark/baselines/debug/rendering-pipeline.baseline.json`

These are bootstrap values and should be replaced with measured medians once benchmark command execution is wired.

### Recommended Validation Sequence

1. Correctness gate
- Run targeted tests from `dev/validation/suite-map.json`.
- Any failed test => final verdict fail.

2. Performance gate
- Run required suites from same mapping.
- Compute delta percent = `(current - baseline) / baseline * 100`.
- Any metric above threshold => suite fail => final verdict fail.

3. Escalation rule
- If renderer core or application loop files are touched, force full mode:
  - all tests
  - both benchmark suites

### Workspace Hygiene Principle

- Test and benchmark runs MUST NOT persist artifacts into the working `content/` tree.
- Any temporary write under `content/` MUST be confined to `content/sandbox-tests/` and removed before process exit.
- Profile and trace outputs MUST be redirected to temporary/sandbox locations and cleaned after run.
- Validation policy is codified in `dev/validation/policy.json` and should be enforced by command implementations.

### Command Examples (Target Workflow)

- `python dev/z1.py test --filter test_render* --config Debug`
- `python dev/z1.py benchmark --suite runtime-core --config Debug`
- `python dev/z1.py benchmark --suite rendering-pipeline --config Debug`
- `python dev/z1.py dcv --auto --config Debug`

### Immediate Next Implementation Tests to Add

1. Add Python tests under `dev/tests/` for:
- dcv auto-detect step mapping
- suite-map path matching and escalation logic
- benchmark baseline comparator (pass/fail/edge cases)

2. Add one integration test fixture that validates:
- command emits single parseable final RESULT line
- fail in benchmark threshold blocks aggregated verdict in dcv

### Deterministic Verification Path

- Fixture-based benchmark suites live under `dev/tests/fixtures/benchmark_suites/`
- Matching baselines live under `dev/tests/fixtures/benchmark_baselines/`
- Trace generator fixture: `dev/tests/fixtures/emit_trace.py`
- Purpose:
  - validate benchmark pass/fail threshold logic deterministically
  - validate report emission without relying on live engine timing noise
  - keep verification isolated from the live `content/` workspace
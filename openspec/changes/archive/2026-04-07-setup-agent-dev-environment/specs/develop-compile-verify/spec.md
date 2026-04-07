## ADDED Requirements

### Requirement: DCV loop must execute after every code modification

Agents MUST run the Develop-Compile-Verify loop after making code changes. The loop is a fixed sequence with gates between steps. All steps are invoked via the Python dev-scripts (`python dev/z1.py <command>`).

#### Scenario: Full DCV loop via unified command
- **WHEN** an agent modifies any source file under `engine/` (`.cpp`, `.h`, `.glsl`, `premake5.lua`)
- **THEN** the agent SHOULD run: `python dev/z1.py dcv --auto`
- **AND** the `--auto` flag detects changed files via `git diff` and enables/skips steps accordingly
- **AND** the agent MAY alternatively run individual steps manually if needed

#### Scenario: Manual DCV step order
- **WHEN** an agent runs DCV steps individually (not via `dcv --auto`)
- **THEN** the steps MUST execute in this order:
  1. GENERATE (conditional)
  2. COMPILE (mandatory)
  3. FORMAT (mandatory)
  4. SHADERS (conditional)
  5. TESTS (conditional)
  6. SMOKE (conditional)
- **AND** each step MUST pass before the next step begins
- **AND** a failing step MUST be fixed and retried, never skipped

### Requirement: GENERATE step -- regenerate project files when build structure changes

#### Scenario: Trigger condition
- **WHEN** any `premake5.lua` file was modified
- **OR** a new `.cpp`/`.h` file was added or removed
- **THEN** the agent MUST run: `python dev/z1.py generate`
- **AND** the RESULT line MUST show `"status": "ok"`

#### Scenario: Skip condition
- **WHEN** only existing source file contents were modified (no new/removed files, no premake changes)
- **THEN** the GENERATE step MAY be skipped

### Requirement: COMPILE step -- build must succeed with zero errors

#### Scenario: Compile execution
- **WHEN** the COMPILE step runs
- **THEN** the agent MUST run: `python dev/z1.py compile`
- **AND** the RESULT line MUST show `"status": "ok"` and `"errors": 0`

#### Scenario: Compile failure
- **WHEN** the RESULT line shows `"status": "fail"` or `"errors"` > 0
- **THEN** the agent MUST read the `[FAIL]` lines to identify errors
- **AND** fix the source code causing the errors
- **AND** re-run `python dev/z1.py compile`
- **AND** repeat until `"errors": 0` is achieved
- **AND** the agent MUST NOT proceed to the next DCV step until compilation succeeds

#### Scenario: Compile warnings
- **WHEN** compilation succeeds with `"errors": 0` but `"warnings"` > 0
- **THEN** the agent SHOULD fix warnings if they are in files the agent modified
- **AND** the agent MAY proceed to the next step even with pre-existing warnings

### Requirement: FORMAT step -- code formatting must be applied

#### Scenario: Format execution
- **WHEN** the FORMAT step runs
- **THEN** the agent MUST run: `python dev/z1.py format`
- **AND** if the output reports files fixed, the agent MUST review them for correctness

### Requirement: SHADERS step -- shader validation when shaders are modified

#### Scenario: Trigger condition
- **WHEN** any `.glsl` file under `engine/content/shader/` was modified or created
- **THEN** the agent MUST run: `python dev/z1.py validate-shaders`
- **AND** all shader compilations MUST show `[OK]` in output

#### Scenario: Shader validation failure
- **WHEN** shader validation reports `[FAIL]` for any shader
- **THEN** the agent MUST fix the shader source
- **AND** re-run `python dev/z1.py compile` (shader_validator may need rebuilding)
- **AND** re-run `python dev/z1.py validate-shaders`
- **AND** the agent MUST NOT proceed until all shaders pass

#### Scenario: Skip condition
- **WHEN** no `.glsl` files were modified in this session
- **THEN** the SHADERS step MAY be skipped

### Requirement: TESTS step -- run relevant tests when testable code changes

#### Scenario: Trigger condition
- **WHEN** code under `engine/runtime/source/` or `engine/bakery/source/` was modified
- **AND** a corresponding `test_*.exe` exists in `engine/bin/test/Debug/`
- **THEN** the agent MUST run: `python dev/z1.py test [--filter <pattern>]`
- **AND** the RESULT line MUST show `"failed": 0`

#### Scenario: Test failure
- **WHEN** the RESULT line shows `"failed"` > 0
- **THEN** the agent MUST read the `[FAIL]` lines to identify which tests failed
- **AND** fix the code if the test reveals a bug introduced by the current change
- **AND** re-run the full DCV loop from COMPILE step after fixing
- **AND** the agent MUST NOT proceed until tests pass

#### Scenario: No relevant tests
- **WHEN** no test executable corresponds to the modified code
- **THEN** the TESTS step MAY be skipped
- **AND** the agent SHOULD note in the task log that no tests cover the changed code

### Requirement: SMOKE step -- editor smoke test after full task groups

#### Scenario: Trigger condition
- **WHEN** a complete task group (as defined in `tasks.md`) is finished
- **THEN** the agent MUST run: `python dev/z1.py smoke`
- **AND** the RESULT line MUST show `"status": "ok"`

#### Scenario: Smoke test failure
- **WHEN** the smoke test reports `"status": "fail"`
- **THEN** the agent MUST investigate (check stderr output in `[FAIL]` lines, recent changes)
- **AND** fix the issue and re-run the full DCV loop from COMPILE
- **AND** the agent MUST NOT proceed to the next task group until smoke passes

#### Scenario: Skip condition for individual tasks
- **WHEN** an individual task within a group is completed (not the final task in the group)
- **THEN** the SMOKE step MAY be skipped (COMPILE + FORMAT are sufficient)

### Requirement: Agents must parse RESULT lines for pass/fail decisions

All dev-script commands produce a machine-parseable RESULT line as their final output line.

#### Scenario: RESULT line parsing
- **WHEN** a dev-script command completes
- **THEN** the agent MUST read the last line of output
- **AND** it MUST match the format: `RESULT: {"status": "ok|fail|warn", ...}`
- **AND** the agent MUST use the `"status"` field (not exit code alone) to determine pass/fail
- **AND** on `"fail"` status, the agent MUST read `[FAIL]` prefixed lines for error details

#### Scenario: No shell-mangling workarounds needed
- **WHEN** an agent invokes a dev-script
- **THEN** it MUST use `python dev/z1.py <command>` directly
- **AND** no `cmd //c` wrapper is needed
- **AND** no backslash escaping or MSBuild flag mangling is needed
- **AND** the Python scripts handle all Windows-specific subprocess invocation internally

### Requirement: DCV loop results must be logged

#### Scenario: DCV completion via unified command
- **WHEN** `python dev/z1.py dcv` completes
- **THEN** it MUST output per-step status lines:
  ```
  [OK] generate
  [OK] compile (0 errors, 2 warnings, 14.1s)
  [OK] format (3 files fixed)
  [SKIP] validate-shaders (no .glsl changes)
  [OK] test (7 passed, 0 failed)
  [SKIP] smoke (not end of task group)
  RESULT: {"status": "ok", "command": "dcv", "steps": {"generate": "ok", "compile": "ok", "format": "ok", "validate-shaders": "skip", "test": "ok", "smoke": "skip"}}
  ```

#### Scenario: DCV manual log
- **WHEN** an agent runs DCV steps manually (not via `dcv` command)
- **THEN** the agent MUST log a summary after all steps complete:
  ```
  DCV: compile ok | format ok | shaders skipped | test ok (test_render_graph) | smoke ok
  ```

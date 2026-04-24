## ADDED Requirements

### Requirement: All dev tooling must be Python-based with a unified CLI entry point

All development scripts MUST be implemented in Python and accessible through a single entry point: `python dev/z1.py <command>`.

#### Scenario: Unified CLI invocation
- **WHEN** an agent or developer needs to run a dev command
- **THEN** it MUST be invocable via `python dev/z1.py <command> [options]`
- **AND** running `python dev/z1.py` with no arguments MUST print available commands with one-line descriptions
- **AND** running `python dev/z1.py <command> --help` MUST print command-specific usage

#### Scenario: Command discovery
- **WHEN** `python dev/z1.py` is run without arguments
- **THEN** it MUST output a command list in the format:
  ```
  z1 dev tools
  
  Commands:
    generate          Regenerate VS project files from premake5
    compile           Build the solution (Debug|x64 by default)
    format            Format source code (tabs, trailing whitespace, CRLF)
    validate-shaders  Validate all GLSL shaders
    test              Discover and run test executables
    benchmark         Run benchmark suites and compare baselines
    smoke             Run editor smoke test (--one-frame)
    dcv               Full develop-compile-verify loop
    release           Package release folder
  ```

### Requirement: All commands must produce structured, agent-friendly output

Every command MUST produce output that agents can parse without reading the full log.

#### Scenario: Status line prefixes
- **WHEN** a command produces status output
- **THEN** each status line MUST be prefixed with one of:
  - `[OK]` -- step succeeded
  - `[FAIL]` -- step failed (blocks progress)
  - `[WARN]` -- non-fatal issue detected
  - `[SKIP]` -- step was skipped (with reason)
  - `[INFO]` -- informational message
  - `[RUN]` -- step starting execution

#### Scenario: Summary line
- **WHEN** a command completes (success or failure)
- **THEN** the final line of output MUST be a machine-parseable summary:
  ```
  RESULT: {"status": "ok|fail|warn", "command": "<name>", "errors": N, "warnings": N, "elapsed": "Xs", "detail": "<brief>"}
  ```
- **AND** the `RESULT:` prefix MUST be on the final line exactly once
- **AND** agents MUST be able to parse the result by reading only the last line

#### Scenario: Error reporting
- **WHEN** a build or validation error occurs
- **THEN** each error MUST be reported on its own line with format:
  ```
  [FAIL] <file>:<line>: <error message>
  ```
- **AND** the error count MUST appear in the summary JSON

### Requirement: All commands must use normalized exit codes

#### Scenario: Exit code convention
- **WHEN** a command finishes
- **THEN** the exit code MUST be:
  - `0` -- success (no errors)
  - `1` -- build/compilation error
  - `2` -- validation error (shaders, format)
  - `3` -- runtime error (smoke test crash, test failure)
  - `4` -- configuration error (missing tools, bad paths)

### Requirement: Input must be normalized across shell environments

All commands MUST accept input in any reasonable format without requiring shell-specific workarounds.

#### Scenario: Path normalization
- **WHEN** a file path is provided as an argument
- **THEN** both forward slashes (`/`) and backslashes (`\`) MUST be accepted
- **AND** both relative and absolute paths MUST be accepted
- **AND** paths MUST be normalized internally to the OS-native format

#### Scenario: Configuration name normalization
- **WHEN** a build configuration is specified (e.g., `--config debug`)
- **THEN** the name MUST be case-insensitive (`debug`, `Debug`, `DEBUG` all map to `Debug`)
- **AND** valid configurations are: `Debug`, `Release`, `Profile`

#### Scenario: No shell-mangling workarounds needed
- **WHEN** an agent invokes a dev command
- **THEN** it MUST work with `python dev/z1.py <command>` directly
- **AND** no `cmd //c` wrapper, no path escaping, no MSBuild flag mangling is needed
- **AND** the Python script handles all Windows-specific subprocess invocation internally

### Requirement: `generate` command must wrap premake5

#### Scenario: Generate execution
- **WHEN** `python dev/z1.py generate` is run
- **THEN** it MUST invoke `utils/premake/premake5.exe vs2022 --vs2026`
- **AND** set the working directory to the repository root
- **AND** report `[OK] Project files generated` or `[FAIL] premake5 failed: <reason>`

### Requirement: `compile` command must build and parse MSBuild output

#### Scenario: Compile execution
- **WHEN** `python dev/z1.py compile [--config Debug]` is run
- **THEN** it MUST invoke `devenv.com z1engine.sln /Build "<Config>|x64"` via subprocess
- **AND** parse the MSBuild output to extract error count, warning count, and individual errors
- **AND** report each error as `[FAIL] <file>:<line>: <message>`
- **AND** report each warning as `[WARN] <file>:<line>: <message>`
- **AND** the summary MUST include `"errors": N, "warnings": N`

#### Scenario: VS path detection
- **WHEN** the compile command needs the VS installation path
- **THEN** it MUST check common installation paths in order:
  1. `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com`
  2. `C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\devenv.com`
  3. `C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com`
- **AND** if none found, report `[FAIL] Visual Studio 2026 not found` with exit code 4

### Requirement: `format` command must absorb existing format_code.py

#### Scenario: Format execution
- **WHEN** `python dev/z1.py format [--dry-run]` is run
- **THEN** it MUST perform all tasks from the current `utils/format_code.py`: tabs conversion, trailing whitespace removal, CRLF enforcement, ASCII check
- **AND** report each modified file as `[INFO] Fixed: <filepath>`
- **AND** report non-ASCII warnings as `[WARN] Non-ASCII: <filepath>:<line>`
- **AND** the summary MUST include `"files_fixed": N`

### Requirement: `validate-shaders` command must report per-shader results

#### Scenario: Shader validation execution
- **WHEN** `python dev/z1.py validate-shaders` is run
- **THEN** it MUST invoke `engine/bin/Debug/shader_validator.exe` (or the configured config)
- **AND** parse the output to report per-shader pass/fail
- **AND** report each failure as `[FAIL] <shader-file>: <error>`
- **AND** report each success as `[OK] <shader-file>`

### Requirement: `test` command must discover and run test executables

#### Scenario: Test discovery
- **WHEN** `python dev/z1.py test [--filter <pattern>]` is run
- **THEN** it MUST scan `engine/bin/test/Debug/` for `test_*.exe` files
- **AND** if `--filter` is provided, only run tests matching the glob pattern
- **AND** if no filter, run all discovered tests

#### Scenario: Test execution and reporting
- **WHEN** tests are run
- **THEN** each test MUST be run as a subprocess
- **AND** report `[OK] test_<name>` (exit 0) or `[FAIL] test_<name>: exit code N` (non-zero)
- **AND** the summary MUST include `"passed": N, "failed": N, "total": N`

### Requirement: `smoke` command must run editor startup test

#### Scenario: Smoke test execution
- **WHEN** `python dev/z1.py smoke [--frames 10]` is run
- **THEN** it MUST invoke `engine/bin/Debug/game.exe --one-frame=<N>`
- **AND** capture stdout and stderr
- **AND** report `[OK] Editor started and rendered N frames` or `[FAIL] Editor crashed: <stderr snippet>`

### Requirement: `dcv` command must orchestrate the full loop

#### Scenario: DCV orchestration
- **WHEN** `python dev/z1.py dcv [options]` is run
- **THEN** it MUST execute commands in this order:
  1. `generate` (if `--generate` flag or premake files changed)
  2. `compile`
  3. `format`
  4. `validate-shaders` (if `--shaders` flag or `.glsl` files exist in git diff)
  5. `test` (if `--test` flag or test-adjacent code changed)
  6. `benchmark` (if `--benchmark` flag or benchmark-adjacent/runtime code changed)
  7. `smoke` (if `--smoke` flag)
- **AND** stop on first failure with the failing command's exit code
- **AND** report per-step status: `[OK] compile`, `[OK] format`, `[SKIP] shaders (no .glsl changes)`, etc.
- **AND** the summary MUST include results from all steps

#### Scenario: DCV auto-detection
- **WHEN** `python dev/z1.py dcv --auto` is run
- **THEN** it MUST use `git diff --name-only` to detect which files changed
- **AND** auto-enable steps based on changed file types:
  - `.lua` files changed -> enable `generate`
  - `.glsl` files changed -> enable `validate-shaders`
  - `engine/runtime/`, `engine/bakery/` changed -> enable `test`
  - `engine/runtime/`, `engine/editor/`, benchmark config paths changed -> enable `benchmark`
  - any code change -> always run `compile` + `format`

### Requirement: `benchmark` command must run performance suites and compare baselines

#### Scenario: Benchmark execution
- **WHEN** `python dev/z1.py benchmark [--suite <name>] [--update-baseline]` is run
- **THEN** it MUST discover and execute benchmark suites with warmup and repeat sampling
- **AND** compare measured metrics against stored baselines unless `--update-baseline` is set
- **AND** report each suite as `[OK]` or `[FAIL]` with delta percentages

#### Scenario: Benchmark summary reporting
- **WHEN** benchmark execution completes
- **THEN** the final `RESULT:` summary MUST include `"command": "benchmark"`
- **AND** it MUST include `"passed": N`, `"failed": N`, and per-suite regression counts
- **AND** it MUST include baseline comparison metadata needed by gate orchestration

### Requirement: Old batch scripts must be thin wrappers

#### Scenario: Backward compatibility
- **WHEN** a developer runs an old batch script (e.g., `dev\build_vs2026.bat`)
- **THEN** the script MUST call `python dev/z1.py <equivalent-command>`
- **AND** pass through the exit code
- **AND** the script body MUST be no more than 3 lines

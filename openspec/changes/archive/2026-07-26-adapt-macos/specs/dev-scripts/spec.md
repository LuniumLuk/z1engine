## MODIFIED Requirements

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
    generate          Regenerate project files from premake5
    compile           Build the solution
    format            Format source code (tabs, trailing whitespace, CRLF on Windows, LF on macOS)
    validate-shaders  Validate all GLSL shaders
    test              Discover and run test executables
    benchmark         Run benchmark suites and compare baselines
    smoke             Run editor smoke test (--one-frame)
    dcv               Full develop-compile-verify loop
    release           Package release folder
  ```
- **AND** command descriptions MUST NOT reference platform-specific tooling (e.g., no "VS project files", no "MSBuild")

### Requirement: `generate` command must wrap premake5

#### Scenario: Generate execution on Windows
- **WHEN** `python dev/z1.py generate` is run on Windows
- **THEN** it MUST invoke `utils/premake/premake5.exe vs2022 --vs2026`
- **AND** set the working directory to the repository root
- **AND** report `[OK] Project files generated` or `[FAIL] premake5 failed: <reason>`

#### Scenario: Generate execution on macOS
- **WHEN** `python dev/z1.py generate` is run on macOS
- **THEN** it MUST invoke `premake5 gmake2` (from PATH or `utils/premake/premake5`)
- **AND** set the working directory to the repository root
- **AND** report `[OK] Project files generated` or `[FAIL] premake5 failed: <reason>`

### Requirement: `compile` command must build and parse compiler output

#### Scenario: Compile execution on Windows
- **WHEN** `python dev/z1.py compile [--config Debug]` is run on Windows
- **THEN** it MUST invoke `devenv.com z1engine.sln /Build "<Config>|x64"` via subprocess
- **AND** parse the MSBuild output to extract error count, warning count, and individual errors
- **AND** report each error as `[FAIL] <file>:<line>: <message>`
- **AND** report each warning as `[WARN] <file>:<line>: <message>`
- **AND** the summary MUST include `"errors": N, "warnings": N`

#### Scenario: Compile execution on macOS with make
- **WHEN** `python dev/z1.py compile [--config Debug]` is run on macOS
- **THEN** it MUST invoke `make` with the generated Makefile
- **AND** parse the GCC/Clang output to extract error count, warning count, and individual errors
- **AND** report each error as `[FAIL] <file>:<line>: <message>`
- **AND** the summary MUST include `"errors": N, "warnings": N`

#### Scenario: Build tool path detection on macOS
- **WHEN** the compile command needs the build tool path on macOS
- **THEN** it MUST check for `make` on PATH
- **AND** if not found, report `[FAIL] make not found (install Xcode Command Line Tools)` with exit code 4

### Requirement: `test` command must discover and run test executables

#### Scenario: Test discovery on Windows
- **WHEN** `python dev/z1.py test [--filter <pattern>]` is run on Windows
- **THEN** it MUST scan `engine/bin/test/Debug/` for `test_*.exe` files
- **AND** if `--filter` is provided, only run tests matching the glob pattern
- **AND** if no filter, run all discovered tests

#### Scenario: Test discovery on macOS
- **WHEN** `python dev/z1.py test [--filter <pattern>]` is run on macOS
- **THEN** it MUST scan `engine/bin/test/Debug/` for `test_*` files (no extension filter)
- **AND** it MUST exclude directories and non-executable files
- **AND** if `--filter` is provided, only run tests matching the glob pattern

#### Scenario: Test execution and reporting
- **WHEN** tests are run
- **THEN** each test MUST be run as a subprocess
- **AND** report `[OK] test_<name>` (exit 0) or `[FAIL] test_<name>: exit code N` (non-zero)
- **AND** the summary MUST include `"passed": N, "failed": N, "total": N`

### Requirement: `validate-shaders` command must report per-shader results

#### Scenario: Shader validation execution
- **WHEN** `python dev/z1.py validate-shaders` is run
- **THEN** it MUST locate the shader_validator binary appropriate for the host OS (`shader_validator.exe` on Windows, `shader_validator` on macOS)
- **AND** parse the output to report per-shader pass/fail
- **AND** report each failure as `[FAIL] <shader-file>: <error>`
- **AND** report each success as `[OK] <shader-file>`

## ADDED Requirements

### Requirement: `smoke` command must find platform-appropriate executable

#### Scenario: Smoke execution on macOS
- **WHEN** `python dev/z1.py smoke` is run on macOS
- **THEN** it MUST search for `game` and `editor` (no extension) in `engine/bin/<config>/`
- **AND** if neither is found, report `[FAIL] No game or editor found` with exit code 4

### Requirement: `release` command must package platform-appropriate binaries

#### Scenario: Release execution on macOS
- **WHEN** `python dev/z1.py release` is run on macOS
- **THEN** it MUST copy `editor` and `game` (no extension) to the release directory
- **AND** it MUST copy `.dylib` shared libraries (not `.dll`)
- **AND** it MUST copy the Python framework or standard library appropriate for macOS

### Requirement: `format` command must enforce platform-appropriate line endings

#### Scenario: Format execution on macOS
- **WHEN** `python dev/z1.py format` is run on macOS
- **THEN** it MUST convert line endings to LF (not CRLF)
- **AND** all other formatting rules (tabs, trailing whitespace, ASCII) SHALL apply identically

#### Scenario: Format execution on Windows
- **WHEN** `python dev/z1.py format` is run on Windows
- **THEN** the existing CRLF enforcement behavior SHALL be preserved

# macOS build performance

## ADDED Requirements

### Requirement: macOS optimized configurations must compile with line-table debug info

Hybrid and Profile builds on macOS MUST pass `-gline-tables-only` (after premake's `-g`, which it overrides), so that optimized builds keep file/line breakpoints while skipping full type debug info. Debug builds MUST keep full `-g`. The flag MUST be applied through a `system:macosx` filter so Windows/MSVC configs stay unchanged.

#### Scenario: Hybrid build on macOS uses line tables
- **WHEN** projects are generated on macOS and `make config=hybrid game` compiles the runtime project
- **THEN** every compile command MUST contain `-gline-tables-only` and MUST NOT rely on `-g` alone
- **AND** a Debug-config compile command MUST still contain `-g` (no line-tables flag)

#### Scenario: Full debug info escape hatch
- **WHEN** `python dev/z1.py generate --full-symbols` is run on macOS
- **THEN** the generated projects MUST compile Hybrid/Profile with full `-g` (the line-tables flag is omitted)
- **AND** a subsequent plain `generate` MUST restore the default line-tables behaviour

### Requirement: Cold macOS builds must be measurably faster than the recorded baseline

On the reference machine (Intel i5-8257U, 4C/8T, 15 W; Apple clang 17; GNU Make 3.81) and with the documented protocol (clean `engine/intermediate/Hybrid` + `engine/bin/Hybrid`, then `make config=hybrid -j8 game`), a cold Hybrid build MUST complete in at most 300 s wall clock (recorded baseline: 361.5 s), and the runtime project's object directory MUST shrink below 200 MB (recorded baseline: 307 MB).

#### Scenario: Cold build meets the target
- **WHEN** a clean Hybrid build of the `game` target is timed on the reference machine after this change
- **THEN** the wall-clock time MUST be ≤ 300 s
- **AND** the measured value MUST be recorded in `openspec/kb/build.md` together with the baseline

#### Scenario: Object footprint shrinks
- **WHEN** the runtime project has been rebuilt from clean
- **THEN** `engine/intermediate/Hybrid/runtime` MUST be smaller than 200 MB

#### Scenario: Incremental builds do not regress
- **WHEN** one `.cpp` file inside the runtime project is touched and the game target is rebuilt
- **THEN** the build MUST complete in ≤ 25 s (recorded baseline: 25.1 s)

### Requirement: The dev CLI must use ccache when it is available on macOS

`python dev/z1.py compile` on macOS MUST detect `ccache` on `PATH` and, unless disabled, run the build with `CC="ccache clang"`, `CXX="ccache clang++"` and `CCACHE_SLOPPINESS=pch_defines,time_macros` (required by the runtime precompiled header). The command MUST report whether ccache was used, and MUST behave exactly as before when ccache is not installed.

#### Scenario: ccache auto-detected
- **WHEN** `ccache` is installed and `python dev/z1.py compile --config Hybrid` is run on macOS
- **THEN** the build MUST run through ccache and the status output MUST say ccache is enabled
- **AND** the `RESULT:` line MUST include the cache mode

#### Scenario: Warm cache makes a repeat rebuild cheap
- **WHEN** the Hybrid objects are deleted and the same build is run a second time with a warm ccache
- **THEN** the build MUST be measurably faster than the first (cold) build
- **AND** ccache statistics MUST show non-zero hits for engine translation units

#### Scenario: ccache absent or disabled
- **WHEN** `ccache` is not installed, or `--no-ccache` is passed
- **THEN** the build MUST run directly through clang/clang++ exactly as before
- **AND** the output MUST NOT claim ccache is enabled

#### Scenario: Forced ccache is missing
- **WHEN** `--ccache` is passed but `ccache` is not installed
- **THEN** the command MUST fail with exit code 4 and a message describing how to install ccache

### Requirement: macOS compile must default to all logical cores

The macOS build invocation MUST default to `os.cpu_count()` parallel jobs instead of the hard-coded `-j4`, and MUST accept `--jobs N` to override it.

#### Scenario: Default jobs follow the CPU count
- **WHEN** `python dev/z1.py compile` runs on a machine with 8 logical CPUs and no `--jobs` flag
- **THEN** the underlying make invocation MUST use 8 jobs

#### Scenario: Explicit override
- **WHEN** `python dev/z1.py compile --jobs 3` is run
- **THEN** the underlying make invocation MUST use 3 jobs
- **AND** an invalid value MUST fail with exit code 4

### Requirement: macOS build-performance settings must not alter Windows behaviour

All new flags and behaviours MUST be scoped with `system:macosx` filters (premake) or macOS-only code paths (dev CLI), so that VS2026 project generation and MSVC builds are byte-for-byte equivalent in configuration to the previous state.

#### Scenario: Windows generation unaffected
- **WHEN** `premake5.lua` and the per-project premake files are reviewed after this change
- **THEN** every added `buildoptions`/warning/debug-info setting MUST sit inside a `system:macosx` filter
- **AND** no Windows-only setting may be removed or changed

#### Scenario: compile.sh stops hiding warnings
- **WHEN** `compile.sh` is inspected after this change
- **THEN** it MUST NOT inject `-w` into `CFLAGS`/`CXXFLAGS`

### Requirement: macOS compiles must go through the dev CLI make path

`python dev/z1.py compile` on macOS MUST invoke `make config=<lowercase-config> -j<jobs> game` from the repository root (the only build entry point; raw `make` usage by agents is not allowed by the workflow rules), MUST parse clang diagnostics (`<file>:<line>:<col>: error|warning: <message>`) for its error/warning counts, and MUST include the job count and cache mode in its `RESULT:` summary.

#### Scenario: macOS compile reports make results
- **WHEN** `python dev/z1.py compile --config Hybrid` runs on macOS
- **THEN** the invoked command MUST be `make config=hybrid -j<jobs> game`
- **AND** the `RESULT:` line MUST contain the error count, warning count, elapsed time, job count, and cache mode

#### Scenario: Invalid options fail fast
- **WHEN** `--jobs` is passed a non-positive or non-numeric value, `--ccache` is passed while ccache is missing, or `--ccache`/`--no-ccache` are combined
- **THEN** the command MUST fail with exit code 4 and a message naming the offending value or conflict

### Requirement: Build-performance baselines must be documented in the knowledge base

`openspec/kb/build.md` MUST document the macOS build path: the debug-info mode, the ccache integration and its escape hatches, the job-count default, and the recorded baseline/after numbers with the measurement protocol.

#### Scenario: KB reflects the new state
- **WHEN** a reader opens `openspec/kb/build.md`
- **THEN** it MUST contain a macOS section describing flags, jobs, ccache usage (including `--no-ccache`/`--ccache`/`--full-symbols`) and the recorded cold-build numbers

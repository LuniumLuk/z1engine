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
    generate          Regenerate VS project files from premake5
    compile           Build the solution (Debug|x64 by default)
    format            Format source code (tabs, trailing whitespace, CRLF)
    validate-shaders  Validate all GLSL shaders
    test              Discover and run test executables
    benchmark         Run benchmark suites and compare baselines
    smoke             Run editor smoke test (--one-frame)
    dcv               Full develop-compile-verify loop
    release           Package release folder
    gen-pybinds       Regenerate Python bindings from reflected types
  ```

---

### Requirement: `dcv` command must orchestrate the full loop

The `dcv` command MUST run the full develop-compile-verify pipeline as an ordered sequence of steps, auto-detecting which steps apply from the git diff, and stopping on the first failure.

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
  8. `gen-pybinds --check` (if reflection-adjacent headers changed)
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
  - headers containing `REFLECTED_`/`REFLECT_ENUM` declarations changed -> enable `gen-pybinds --check`
  - any code change -> always run `compile` + `format`

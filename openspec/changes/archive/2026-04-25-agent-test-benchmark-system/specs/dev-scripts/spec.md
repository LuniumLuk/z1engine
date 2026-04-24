## MODIFIED Requirements

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
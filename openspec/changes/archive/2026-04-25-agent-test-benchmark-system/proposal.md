## Why

Agent-driven changes currently rely on ad-hoc manual checks. The repository has build and test commands, but no standardized post-modification validation contract that consistently runs both regression tests and performance benchmarks. This leads to uneven quality gates and makes it hard to detect runtime or performance regressions before handoff.

## What Changes

- Define a mandatory post-change validation workflow that runs targeted tests and benchmark suites after agent code modifications.
- Add benchmark command support to the developer CLI so benchmarks run with stable settings, repeat counts, and machine-readable output.
- Introduce a result artifact format that summarizes pass/fail test outcomes and benchmark regressions against a tracked baseline.
- Define selection rules that map changed files to required test and benchmark scopes (fast path for small changes, full path for engine-wide changes).
- Add guardrails for flaky tests and noisy benchmark samples (retry and confidence rules) to avoid false failures.

## Capabilities

### New Capabilities
- `change-validation-gates`: Defines required post-modification validation gates (tests, benchmarks, pass criteria, and reporting).
- `benchmark-suite-management`: Defines benchmark discovery, execution, baseline storage, and regression threshold evaluation.

### Modified Capabilities
- `dev-scripts`: Extends CLI requirements to support benchmark execution and unified validation summary output.
- `standard-workflow`: Updates the develop-compile-verify flow to include mandatory test and benchmark validation after modifications.

## Impact

- Affected systems: `dev/z1.py`, `dev/commands/` workflow orchestration, test runner logic, and new benchmark runner logic.
- New artifacts: benchmark definitions and baseline result files under repository-managed paths.
- Process impact: agent completion criteria become objective and repeatable via a single validation command.
- Risk: benchmark instability on developer machines; mitigated via warmup/repeat rules and threshold windows.
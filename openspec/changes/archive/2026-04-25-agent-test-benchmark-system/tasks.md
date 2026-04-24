## 1. Validation Contract and Reporting

- [x] 1.1 Define validation policy configuration for required test/benchmark gates and failure modes
- [x] 1.2 Implement a unified validation report schema (JSON) with verdict, suite outcomes, and environment metadata
- [x] 1.3 Add persistence of validation reports to a repository-managed output path for review and archival

## 2. Benchmark Command and Baseline Handling

- [x] 2.1 Add `benchmark` command module under `dev/commands/` and register it in `dev/z1.py`
- [x] 2.2 Implement benchmark suite discovery from declarative benchmark metadata
- [x] 2.3 Implement warmup/repeat execution and median/percentile summary calculation
- [x] 2.4 Implement baseline load and threshold comparison logic with per-case delta reporting
- [x] 2.5 Add baseline update flow (`--update-baseline`) with explicit safeguard messaging

## 3. Scope Mapping and Gate Orchestration

- [x] 3.1 Add changed-path to required-suite mapping configuration for tests and benchmarks
- [x] 3.2 Extend `dcv` orchestration to include benchmark step and mapping-based auto-selection
- [x] 3.3 Implement escalation logic that forces full validation for core runtime/renderer/shared changes
- [x] 3.4 Ensure `RESULT:` summary for `dcv` includes aggregated correctness and performance verdicts

## 4. Test and Benchmark Assets

- [x] 4.1 Define initial benchmark suites for high-risk runtime and rendering paths
- [x] 4.2 Add initial baseline files for each benchmark suite in version-controlled paths
- [x] 4.3 Add or update tests needed to cover validation gate policy and scope mapping behavior

## 5. Workflow Integration and Documentation

- [x] 5.1 Update workflow docs and OpenSpec references to require validation gates before completion
- [x] 5.2 Document benchmark authoring, baseline maintenance, and threshold tuning process
- [x] 5.3 Document waiver/override policy for temporary benchmark instability

## 6. Verification

- [x] 6.1 Run `python dev/z1.py test` and confirm structured summary output remains compliant
- [x] 6.2 Run `python dev/z1.py benchmark` on initial suites and validate threshold verdict behavior
- [x] 6.3 Run `python dev/z1.py dcv --auto` on representative changes and verify benchmark inclusion rules
- [x] 6.4 Confirm a failed benchmark threshold blocks final completion verdict
- [x] 6.5 Confirm validation report artifacts contain required fields and are machine-parseable
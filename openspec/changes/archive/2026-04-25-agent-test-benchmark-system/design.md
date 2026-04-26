## Context

The repository already has Python-based dev scripts and a DCV workflow, but there is no explicit requirement that agent-delivered changes must pass both correctness checks and performance checks before completion. Tests can be run, but benchmark execution, baseline comparison, and regression thresholds are not consistently defined.

Constraints:
- Windows x64 only development environment.
- Existing workflow entry point is `python dev/z1.py` and should remain the control surface.
- Validation output must be machine-readable so agents can make deterministic pass/fail decisions.
- Benchmarks must tolerate normal machine noise while still detecting meaningful regressions.

Stakeholders:
- Agent implementers who need deterministic finish criteria.
- Reviewers who need reliable evidence that behavior and performance are preserved.
- Engine maintainers who need low-friction ways to add new tests and benchmarks.

## Goals / Non-Goals

**Goals:**
- Add a mandatory post-change validation contract covering tests and benchmarks.
- Define benchmark suite structure, execution model, and baseline comparison rules.
- Integrate benchmark and test outcomes into a single validation summary that can gate completion.
- Support selective validation for small changes and full validation for broad runtime changes.

**Non-Goals:**
- Building a cloud-distributed benchmarking platform.
- Replacing existing test binaries or migrating all tests to a new framework.
- Guaranteeing cross-machine identical benchmark numbers.
- Defining CI hosting specifics in this change.

## Decisions

1. Single orchestration command for completion gating
- Decision: Extend workflow so one command performs post-modification validation and returns a final machine-readable result.
- Rationale: Reduces missed steps and enables deterministic agent behavior.
- Alternatives considered:
  - Separate manual test and benchmark commands only: rejected because it relies on discipline and is error-prone.
  - Editor-driven validation menu only: rejected because agent workflows need non-interactive CLI.

2. Benchmark baselines stored in-repo as explicit artifacts
- Decision: Store baseline metrics in versioned files under a dedicated benchmark baseline path.
- Rationale: Baselines become reviewable, diffable, and coupled to engine changes.
- Alternatives considered:
  - External baseline service: rejected for operational overhead and local workflow complexity.
  - Hardcoded thresholds without baselines: rejected because absolute timing varies too much by scenario.

3. Threshold-based regression detection with variance controls
- Decision: Require warmup, repeat runs, and median/percentile comparison against baseline, with configurable tolerance bands.
- Rationale: Controls noise while preserving sensitivity to real regressions.
- Alternatives considered:
  - Single-run comparisons: rejected due to high false positives.
  - Average-only comparisons: rejected because outliers can distort means.

4. Change-scope mapping determines required suites
- Decision: Define mapping rules from changed paths/modules to required test and benchmark suites, with escalation to full suites for core runtime changes.
- Rationale: Keeps feedback fast on localized changes and comprehensive on risky changes.
- Alternatives considered:
  - Always run full suites: rejected due to long cycle times.
  - Only run user-selected suites: rejected because it can miss regressions.

5. Unified report artifact for handoff evidence
- Decision: Emit a structured validation report containing test pass/fail, benchmark deltas, threshold results, and environment metadata.
- Rationale: Makes reviewer verification and archival straightforward.
- Alternatives considered:
  - Human-readable logs only: rejected because parsing is inconsistent.
  - Separate reports per step only: rejected because gate evaluation needs one summary verdict.

## Risks / Trade-offs

- [Benchmark variance across developer machines] -> Use warmup, repeats, median-based comparisons, and per-benchmark tolerance windows.
- [Validation time increase] -> Use scope-based selection by default; reserve full-suite runs for broad changes or release checks.
- [Baseline drift and stale thresholds] -> Require explicit baseline update workflow with rationale in changelists.
- [False confidence if mappings are incomplete] -> Start with conservative path-to-suite mappings and expand coverage over time.
- [Agent overfitting to threshold numbers] -> Record contextual metadata (config, build type, scene) in report to prevent blind tuning.

## Migration Plan

1. Add benchmark command and baseline loader/comparator in dev scripts.
2. Add validation orchestration updates and report output schema.
3. Introduce initial benchmark suites and baseline files for high-risk subsystems.
4. Update workflow docs/specs so completion requires passing validation report.
5. Roll out in phases: warn-only benchmark mode first, then enforcing mode.

Rollback strategy:
- Keep benchmark gate configurable. If instability is discovered, downgrade benchmark failures to warnings while retaining test gates.
- Preserve previous workflow command behavior via compatibility flags during transition.

## Open Questions

- Which benchmark scenes/workloads should be mandatory for first-phase enforcement?
- What default regression threshold is acceptable per benchmark type (CPU frame time, GPU pass, memory)?
- Should benchmark gate enforce absolute max frame time caps in addition to baseline-relative deltas?
- How should baseline updates be reviewed and approved for intentional performance trade-offs?
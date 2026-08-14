## Context

`python dev/z1.py benchmark` and the `dcv` validation gate exist in `dev/commands/` and are exercised by the test/CI workflow. Benchmark suites are declared as JSON in `dev/benchmark/suites/`, baselines stored per configuration, and regression tolerances configured per metric.

## Goals / Non-Goals

**Goals:**
- Declarative benchmark suites discoverable by the CLI.
- Warmup + repeats with median/percentile stats and environment capture.
- Baseline comparison classifying regressions via thresholds, wired into `dcv`.

**Non-Goals:**
- Real-time in-application profiler (out of scope for this change).

## Decisions

### D1. JSON-declared suites + per-config baselines
Suites live in `dev/benchmark/suites/*.json`; baselines in `dev/benchmark/baselines/<config>/*.baseline.json`; `dev/validation/suite-map.json` maps suites to triggering code areas.

### D2. Benchmarks gate the DCV loop
The `dcv` step order (generate, compile, format, validate-shaders, test, benchmark, smoke) runs benchmarks when mapped suites are affected, failing on regressions over configured thresholds.

## Risks / Trade-offs

- [Machine noise in timings] → Warmup + repeats + median statistics + environment metadata.
- [Unstable baselines] → Waiver mechanism in `dev/validation/policy.json` with documented owner/expiry.

## Migration Plan

Already shipped; no migration required.

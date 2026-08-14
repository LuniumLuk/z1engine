## Why

Performance work needs repeatable measurement and a validation gate to catch regressions. This change introduces a declarative benchmark-suite system and wires it (plus shader/test validation) into the `dcv` develop-compile-verify loop.

## What Changes

- `python dev/z1.py benchmark --suite <name>` runs declarative benchmark suites defined in `dev/benchmark/suites/*.json`, with warmup + repeated sampling, environment capture, and baseline comparison in `dev/benchmark/baselines/<config>/*.baseline.json`
- `dev/validation/` holds `suite-map.json` (which suites map to which code changes) and `policy.json` (artifact/waiver policy)
- `dcv` runs generate → compile → format → validate-shaders → test → benchmark → smoke with step selection

## Capabilities

### New Capabilities

- `benchmark-suite-management`: Declarative, discoverable benchmark suites with variance controls and threshold-based baseline regression classification. (Already present in `openspec/specs/benchmark-suite-management/`; this change records the work that produced it.)

## Impact

- `dev/z1.py` + `dev/commands/benchmark.py`, `dcv.py`, `dev/benchmark/`, `dev/validation/`
- Committed as `54a0bd0 feat: add validation and benchmark gates`

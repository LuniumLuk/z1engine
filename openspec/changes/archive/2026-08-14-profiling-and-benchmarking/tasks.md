## 1. Benchmark infrastructure

- [x] 1.1 Add `python dev/z1.py benchmark --suite <name>` with declarative JSON suite discovery (`dev/benchmark/suites/`)
- [x] 1.2 Implement warmup + repeated sampling with median/percentile summary and environment capture
- [x] 1.3 Implement baseline storage (`dev/benchmark/baselines/<config>/*.baseline.json`) and threshold-based regression classification
- [x] 1.4 Add `dev/validation/suite-map.json` (suite → code-area mapping) and `policy.json` (artifact hygiene + waiver policy)

## 2. Validation gate integration

- [x] 2.1 Add `python dev/z1.py dcv` running generate → compile → format → validate-shaders → test → benchmark → smoke with step selection
- [x] 2.2 Wire benchmarks into DCV so mapped suites run when relevant code changes and regressions fail the gate

## 3. Validation

- [x] 3.1 `python dev/z1.py benchmark` runs suites and compares against baselines
- [x] 3.2 `python dev/z1.py dcv --auto` executes the full loop

## ADDED Requirements

### Requirement: Benchmark suites MUST be declarative and discoverable
The system SHALL define benchmark suites via repository-managed metadata so tooling can discover and execute them deterministically.

#### Scenario: Discovery
- **WHEN** the benchmark command is run without explicit suite arguments
- **THEN** it MUST discover all enabled suites from the benchmark metadata path
- **AND** execution order MUST be deterministic

#### Scenario: Explicit selection
- **WHEN** specific suites are requested
- **THEN** only those suites MUST run
- **AND** unknown suite identifiers MUST produce a validation error

### Requirement: Benchmarks MUST run with variance controls
Benchmark execution SHALL include warmup and repeat sampling to reduce machine-noise effects.

#### Scenario: Sampling model
- **WHEN** a benchmark suite executes
- **THEN** each benchmark case MUST run at least one warmup pass and multiple measured passes
- **AND** summary statistics MUST include median or percentile metrics

#### Scenario: Environment capture
- **WHEN** benchmark results are produced
- **THEN** the system MUST capture environment metadata (configuration, machine id hash, GPU/CPU identifiers where available)
- **AND** include it in result output for interpretability

### Requirement: Baseline comparisons MUST classify regressions with thresholds
Benchmark results SHALL be compared against stored baselines using configured tolerance bands.

#### Scenario: Regression detected
- **WHEN** measured metric exceeds allowed regression threshold relative to baseline
- **THEN** the benchmark case MUST be marked failed
- **AND** the report MUST include baseline value, measured value, delta percent, and threshold

#### Scenario: Intentional baseline updates
- **WHEN** maintainers approve an intentional performance trade-off
- **THEN** baseline files MUST be updated in-repo with rationale in change history
- **AND** subsequent validations MUST compare against the updated baseline

### Requirement: Benchmark command MUST emit structured summaries
Benchmark execution SHALL emit a machine-readable summary compatible with the broader validation gate report.

#### Scenario: Command result line
- **WHEN** benchmark command completes
- **THEN** it MUST emit one final structured summary line containing suite totals, pass/fail counts, and aggregate regression verdict
- **AND** this summary MUST be consumable by gate orchestration without additional parsing heuristics
## MODIFIED Requirements

### Requirement: Implementation must follow tasks sequentially with validation gates
Implementation MUST proceed through tasks in order. Each task group must pass compilation and shader validation before the next begins.

#### Scenario: Build gate between task groups
- **WHEN** a numbered task group in `tasks.md` is completed
- **THEN** `python dev/z1.py compile` MUST be run and MUST succeed with zero errors before proceeding to the next task group

#### Scenario: Shader validation gate
- **WHEN** a task modifies any `.glsl` file
- **THEN** `python dev/z1.py validate-shaders` MUST be run and MUST pass before proceeding

#### Scenario: Benchmark validation gate for performance-sensitive changes
- **WHEN** a task modifies performance-sensitive runtime or rendering paths
- **THEN** `python dev/z1.py benchmark --suite <mapped-suite-set>` MUST be run and MUST pass configured regression thresholds before proceeding
- **AND** benchmark deltas MUST be recorded in the validation report

#### Scenario: Final verification
- **WHEN** all task groups are complete
- **THEN** `python dev/z1.py dcv --auto` MUST succeed end-to-end
- **AND** if benchmark suites are required by change-scope mapping, `dcv --auto` MUST include benchmark execution before returning success
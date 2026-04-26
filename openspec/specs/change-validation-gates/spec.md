## ADDED Requirements

### Requirement: Agent completion MUST require a validation gate run
Any agent-authored repository modification SHALL be considered complete only after a validation gate command executes and returns a passing verdict.

#### Scenario: Completion gate enforced
- **WHEN** an agent finishes code changes
- **THEN** the agent MUST execute the configured validation gate command before declaring completion
- **AND** the gate result MUST be persisted in a structured report artifact

#### Scenario: Failure blocks completion
- **WHEN** the validation gate reports any failed test or failed performance threshold
- **THEN** completion status MUST be `fail`
- **AND** the change MUST NOT be marked complete until failures are resolved or explicitly waived

### Requirement: Validation gates MUST combine correctness and performance signals
The validation system SHALL evaluate both regression tests and benchmark thresholds in one decision model.

#### Scenario: Combined verdict
- **WHEN** validation completes
- **THEN** the final verdict MUST consider test pass/fail outcomes and benchmark regression checks
- **AND** the verdict MUST expose separate sections for correctness and performance in the output report

#### Scenario: Missing benchmark data handling
- **WHEN** a required benchmark has no baseline or no runnable target
- **THEN** validation MUST return `warn` or `fail` according to policy configuration
- **AND** the report MUST identify the missing data reason

### Requirement: Change-scope mapping MUST select required suites
Validation SHALL determine which tests and benchmarks are required based on changed paths and subsystem mappings.

#### Scenario: Scoped validation for localized changes
- **WHEN** only files mapped to a single subsystem are modified
- **THEN** only suites mapped to that subsystem MUST be required
- **AND** core sanity suites MUST still run

#### Scenario: Escalation to full validation
- **WHEN** core runtime, renderer, or shared foundation paths are modified
- **THEN** full test and benchmark suites MUST be required
- **AND** the report MUST state that full mode was auto-selected

### Requirement: Validation report MUST be machine-readable and archiveable
Validation output SHALL include a single machine-readable summary with deterministic keys and a stable schema.

#### Scenario: Report schema
- **WHEN** validation finishes
- **THEN** the final report MUST include: command id, timestamp, environment, suites run, passed/failed counts, benchmark deltas, thresholds, and final verdict
- **AND** the report MUST be consumable by automated tooling without parsing free-form logs

#### Scenario: Historical traceability
- **WHEN** a change is reviewed later
- **THEN** reviewers MUST be able to inspect the saved validation report and identify which suites and thresholds produced the verdict
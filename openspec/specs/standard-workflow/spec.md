## ADDED Requirements

### Requirement: Change lifecycle must follow the spec-driven artifact sequence

Every openspec change MUST produce artifacts in the following order: proposal, design, specs, tasks. No artifact may be created before its predecessors are complete.

#### Scenario: Proposal first
- **WHEN** a new change is created via `openspec new change`
- **THEN** the first artifact produced MUST be `proposal.md`
- **AND** it MUST contain at minimum: Why, What Changes, Capabilities, and Impact sections

#### Scenario: Design after proposal
- **WHEN** `proposal.md` is complete
- **THEN** `design.md` MUST be created next
- **AND** it MUST contain: Context, Goals / Non-Goals, Decisions (with alternatives considered), and Risks / Trade-offs sections

#### Scenario: Specs after design
- **WHEN** `design.md` is complete
- **THEN** one or more spec files MUST be created under `specs/<capability-name>/spec.md`
- **AND** each spec MUST contain ADDED (or MODIFIED) Requirements with WHEN/THEN scenarios

#### Scenario: Tasks after specs
- **WHEN** all spec files are complete
- **THEN** `tasks.md` MUST be created
- **AND** each task MUST reference the file(s) it modifies
- **AND** the final task group MUST be a verification section that includes build, shader validation, and runtime checks

### Requirement: Implementation must follow tasks sequentially with validation gates

Implementation MUST proceed through tasks in order. Each task group must pass compilation and shader validation before the next begins.

#### Scenario: Build gate between task groups
- **WHEN** a numbered task group in `tasks.md` is completed
- **THEN** `python dev/z1.py compile` MUST be run and MUST succeed with zero errors before proceeding to the next task group

#### Scenario: Shader validation gate
- **WHEN** a task modifies any `.glsl` file
- **THEN** `python dev/z1.py validate-shaders` MUST be run and MUST pass before proceeding

#### Scenario: Final verification
- **WHEN** all task groups are complete
- **THEN** `python dev/z1.py dcv --auto` MUST succeed end-to-end

### Requirement: DCV loop must be used instead of raw batch scripts

All build, test, and validation operations MUST go through the Python dev-scripts CLI, not raw batch files or direct tool invocations.

#### Scenario: Agent runs build
- **WHEN** an agent needs to compile the project
- **THEN** it MUST use `python dev/z1.py compile [--config <Config>]`
- **AND** it MUST NOT invoke `devenv.com` or `MSBuild` directly
- **AND** it MUST NOT use `cmd //c` wrappers for build commands

#### Scenario: Agent runs validation
- **WHEN** an agent needs to validate shaders
- **THEN** it MUST use `python dev/z1.py validate-shaders`
- **AND** it MUST NOT invoke `shader_validator.exe` directly

#### Scenario: Agent runs tests
- **WHEN** an agent needs to run tests
- **THEN** it MUST use `python dev/z1.py test [--filter <pattern>]`
- **AND** it MUST NOT invoke test executables directly

#### Scenario: Agent runs full verification
- **WHEN** an agent needs to verify a complete change
- **THEN** it MUST use `python dev/z1.py dcv --auto`
- **AND** it MUST parse the final `RESULT:` JSON line to determine pass/fail

### Requirement: Completed changes must be archived

After all tasks are implemented and verification passes, the change MUST be archived.

#### Scenario: Archive on completion
- **WHEN** all tasks in `tasks.md` are marked complete and verification passes
- **THEN** the change directory MUST be moved from `openspec/changes/<name>/` to `openspec/changes/archive/<name>/`
- **AND** `openspec status` MUST show no active changes for that entry

### Requirement: Coding style compliance

All code changes MUST follow the project coding style defined in `openspec/kb/coding-style.md`.

#### Scenario: Style rules applied
- **WHEN** code is written or modified as part of a change
- **THEN** it MUST use hard tabs for indentation
- **AND** K&R brace style (opening brace on same line)
- **AND** `m_` prefix for member variables
- **AND** PascalCase for type names, camelCase for methods
- **AND** `Type const&` const placement (east const)
- **AND** `pch.h` as the first include in `.cpp` files
- → see [agent-code-regulations/spec.md] for complete rules

### Requirement: Proposal must declare deferred items explicitly

Changes that intentionally exclude related work MUST document it.

#### Scenario: Deferred section present
- **WHEN** a proposal omits work that a reviewer might reasonably expect
- **THEN** `proposal.md` MUST contain a "Deferred" subsection under "What Changes" listing each omitted item with a brief rationale

### Requirement: Build commands must use Python dev-scripts on Windows

The project builds on Windows with VS2026/MSBuild. All build invocations MUST go through `python dev/z1.py` which handles Windows-specific subprocess quirks internally.

#### Scenario: No shell-mangling workarounds needed
- **WHEN** an agent invokes a build command
- **THEN** it MUST use `python dev/z1.py <command>` directly
- **AND** no `cmd //c` wrapper, no path escaping, no MSBuild flag mangling is needed
- **AND** the Python script handles all Windows-specific subprocess invocation internally
- → see [dev-scripts/spec.md] for complete command reference

#### Scenario: Path separator conventions
- **WHEN** constructing paths for dev-script commands
- **THEN** both forward slashes (`/`) and backslashes (`\`) are accepted
- **AND** the Python scripts normalize paths internally

### Requirement: Knowledge base must be consulted and updated every session

#### Scenario: KB protocol
- **WHEN** any agent session starts
- **THEN** the agent MUST read `openspec/kb/index.md` and relevant topic files
- **AND** at session end, the agent MUST check if KB needs updates
- → see [agent-knowledge-base/spec.md] for full protocol

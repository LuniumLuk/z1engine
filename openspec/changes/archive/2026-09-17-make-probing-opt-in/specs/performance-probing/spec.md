## MODIFIED Requirements

### Requirement: Compile-time gating

The probing system SHALL be compiled in only when `ENABLE_PROBING` is defined and SHALL compile to no-ops
(no timing code, no GL calls, no logging) when it is not. `ENABLE_PROBING` SHALL be defined only when the
`Hybrid` configuration is built from projects generated with the premake5 `--probing` option
(`./generate.sh --probing`, `python dev/z1.py generate --probing`, `generate_vs2026.bat --probing`);
generation without that option SHALL produce probe-free projects for every configuration, and project
generation SHALL report whether probing was enabled. The existing `ENABLE_PROFILE` chrome-trace
instrumentation SHALL remain independent; probing is a separate, lightweight CPU+GPU budget facility.

#### Scenario: Probing disabled
- **WHEN** the project is built without `ENABLE_PROBING` (the default: generated without `--probing`)
- **THEN** all probe macros compile to empty statements and no probing code or GPU queries are executed

#### Scenario: Probing enabled
- **WHEN** projects are generated with `--probing` and the Hybrid configuration is built
- **THEN** frames are measured on CPU and GPU and `[probe]` reports are logged

#### Scenario: Generation reports the probing state
- **WHEN** `generate` runs, with or without `--probing`
- **THEN** the command output states whether `ENABLE_PROBING` was enabled for the Hybrid configuration

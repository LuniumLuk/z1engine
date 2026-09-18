# performance-probing Specification

## Purpose
TBD - created by archiving change add-probing-and-quality-presets. Update Purpose after archive.
## Requirements
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

### Requirement: CPU frame budgets

With probing enabled the system SHALL measure, per frame, the wall-clock durations of the main loop phases
and record them under stable scope names: `frame` (loop total), `update` (layer stack), `imgui` (with
sub-scopes `imgui_begin`, `imgui_drawdata`, `imgui_platform`, `imgui_end`), `window_update` (with sub-scope
`glfw_poll`), `swap`. Editor builds SHALL additionally record `scene_update`, `renderer_draw`, `picking`.
The system SHALL record the per-frame draw call count as `draws`.

#### Scenario: Report contains CPU scopes
- **WHEN** a report interval elapses with probing enabled
- **THEN** a log line per scope reports average, min, max over the interval, together with the frame total
  and the implied fps

#### Scenario: Summary at shutdown
- **WHEN** the application shuts down with probing enabled
- **THEN** a summary of per-scope run averages is logged

### Requirement: GPU frame timing

The probing system SHALL record GPU frame time from a ring of `GL_TIME_ELAPSED` timer queries that cover the
frame's entire GL command stream. Results SHALL be read back without blocking the pipeline (the sample from
an earlier frame is resolved after `GL_QUERY_RESULT_AVAILABLE` reports true) and recorded under the scope
name `gpu`. A sample that never becomes available SHALL be skipped and counted, never waited on.

#### Scenario: GPU sample available
- **WHEN** the query for a previously rendered frame reports its result as available
- **THEN** its duration in milliseconds is recorded as the `gpu` scope for that frame

#### Scenario: Driver never reports availability (macOS trait)
- **WHEN** `GL_QUERY_RESULT_AVAILABLE` remains false for a query (known Apple GL behaviour)
- **THEN** probing skips the sample, counts it as a miss reported in the periodic diagnostics, and does not
  stall or sync the frame

#### Scenario: GPU probing disabled at runtime
- **WHEN** `Z1_PROBE_NO_GPU=1` is set in the environment
- **THEN** no timer queries are created or issued

### Requirement: Reports, environment info and diagnostic knobs

The system SHALL log a report every `Z1_PROBE_EVERY` frames (default 120; `Z1_PROBE_QUIET=1` suppresses
periodic reports but keeps the summary). After the first frame it SHALL log a one-time environment line with
window size, framebuffer size, content scale, monitor mode/refresh and GL renderer strings — read after
rendering so the transient pre-first-frame framebuffer reading on macOS is avoided. Diagnostic knobs SHALL be
supported only in probing builds: `Z1_PROBE_VSYNC=0|1` (force swap interval), `Z1_PROBE_FINISH=1` (record a
timed `glFinish()` drain as `gpu_drain` before present), `Z1_PROBE_NO_GPU=1`, `Z1_NO_GL_CHECK=1` (disable
`glGetError()` polling).

#### Scenario: Periodic report
- **WHEN** 120 frames have completed since the last report
- **THEN** one report block (fps header plus per-scope lines) is logged

#### Scenario: Quiet mode
- **WHEN** `Z1_PROBE_QUIET=1` is set
- **THEN** only the one-time environment line and the final summary are logged

#### Scenario: Vsync override
- **WHEN** `Z1_PROBE_VSYNC=0|1` is set at launch
- **THEN** the swap interval is forced to that value and the periodic report reflects the resulting pacing

#### Scenario: Error-check A/B
- **WHEN** `Z1_NO_GL_CHECK=1` is set in a probing build
- **THEN** `glGetError()` polling is skipped so its cost can be measured


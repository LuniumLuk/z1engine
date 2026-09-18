# frame-loop-robustness Specification

## Purpose
TBD - created by archiving change add-probing-and-quality-presets. Update Purpose after archive.
## Requirements
### Requirement: Vsync state is applied to the GL context at startup

When the GL context is initialized, the engine SHALL apply the window's advertised vsync state via
`glfwSwapInterval` (enabled → 1, disabled → 0) so that the setting surfaced in the UI matches the actual swap
behavior. Runtime toggling SHALL keep working unchanged, and the probing vsync override (when probing is
compiled in and the environment variable is set) SHALL take precedence.

#### Scenario: Startup applies the advertised state
- **WHEN** the engine starts with vsync enabled in the window state (the default)
- **THEN** `glfwSwapInterval(1)` is applied before the first frame is presented

#### Scenario: Runtime toggle unchanged
- **WHEN** the user toggles vsync in the editor debug panel
- **THEN** the swap interval changes immediately and the window state reflects the new value

#### Scenario: Probing override wins
- **WHEN** the probing build runs with `Z1_PROBE_VSYNC=0`
- **THEN** the swap interval ends up at 0 regardless of the window state

### Requirement: Render loop tolerates an absent display

While no display monitor is present (for example while the macOS display is asleep), the application SHALL
keep running without crashing: layer updates and ImGui rendering SHALL be skipped for those frames (avoiding
the ImGui multi-viewport monitor-list assertion), and the loop SHALL resume normal rendering automatically
as soon as a monitor is available again.

#### Scenario: Display sleeps during a session
- **WHEN** the platform reports zero monitors while the application is running
- **THEN** no ImGui frame is begun and no layer update is executed, and the process does not abort

#### Scenario: Display wakes
- **WHEN** a monitor is reported again
- **THEN** the next frame renders normally with no restart or state loss

#### Scenario: Window minimized (unchanged behavior)
- **WHEN** the window framebuffer size is zero
- **THEN** the existing minimized path skips rendering as before


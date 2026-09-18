# graphics-quality-presets Specification

## Purpose
TBD - created by archiving change add-probing-and-quality-presets. Update Purpose after archive.
## Requirements
### Requirement: Editor quality preset selection

The editor SHALL provide a quality preset selector with exactly three levels — LOW, MEDIUM, HIGH — in the
global settings panel. Selecting a level SHALL apply it immediately by calling the dedicated preset
application function. The preset mechanism SHALL NOT be part of the reflection system (no `REFLECTED_FIELD`)
and SHALL NOT be compiled into game builds.

#### Scenario: Selecting a preset in the panel
- **WHEN** the user selects MEDIUM in the global settings panel
- **THEN** `apply_quality_preset` runs and the corresponding global settings take effect on the next frame

#### Scenario: Game builds have no presets
- **WHEN** the game executable is built and run
- **THEN** no preset UI or preset application code is present; render settings come only from scene data and
  scripts

### Requirement: Preset definitions

The presets SHALL map to the following render settings and SHALL NOT modify artistic values (e.g. AO radius,
exposure, gamma, sun parameters):

| Setting | LOW | MEDIUM | HIGH |
|---|---|---|---|
| `taa_enabled` / `taa_sharpen_enabled` | off / off | on / off | on / on |
| `ao_enabled` / `ao_resolution` / `ao_blur_enabled` | on / quarter / on | on / half / on | on / half / on |
| `pp_bloom_enabled` | off | on | on |
| `ssr_enabled` | off | unchanged | unchanged |
| `sm_resolution` / `sm_cascade_count` | 1024 / 1 | 2048 / 2 | 2048 / 4 |

#### Scenario: LOW disables expensive features
- **WHEN** LOW is applied
- **THEN** TAA (and its sharpen), bloom and SSR are disabled and the shadow map runs at 1024² with one
  cascade, while AO stays enabled at quarter resolution

#### Scenario: HIGH matches the previous defaults
- **WHEN** HIGH is applied
- **THEN** TAA with sharpen, half-resolution AO, bloom, and 2048² × 4 cascades shadows are enabled, matching
  the engine's previous default configuration

#### Scenario: Artistic settings untouched
- **WHEN** any preset is applied
- **THEN** settings such as `ao_radius`, `ao_intensity`, `pp_exposure`, `pp_gamma` and sun parameters keep
  their scene-authored values

### Requirement: Preset persistence

The editor settings file (`editor_settings.yaml`) SHALL persist the selected preset as `quality_preset`
(integer; absent = HIGH). The stored preset SHALL be applied once at editor startup, after the initial scene
load, and SHALL NOT be re-applied when other scenes are subsequently loaded (their stored settings remain
authoritative until the user selects a preset again).

#### Scenario: Startup applies the stored preset
- **WHEN** the editor starts and `editor_settings.yaml` contains `quality_preset: 0` (LOW)
- **THEN** the LOW settings are applied after the initial scene has been loaded

#### Scenario: Scene load does not clobber with the preset
- **WHEN** the user opens another scene after startup without touching the selector
- **THEN** that scene's stored global settings are used as-is

### Requirement: Preset toggles take effect in the render graph

Turning TAA off SHALL skip the velocity, TAA and TAA-sharpen passes and route the post-process pass directly
from the scene-color output. Turning bloom off SHALL skip the bloom downsample/upsample chain and the
post-process pass SHALL not depend on bloom outputs. Shadow settings SHALL be re-applied to renderer
resources when changed.

#### Scenario: TAA disabled skips passes
- **WHEN** a frame is rendered with `taa_enabled` false
- **THEN** the render graph contains no velocity, TAA or TAA-sharpen pass and the post-process pass consumes
  the scene-color (or SSR output) directly

#### Scenario: Bloom disabled skips passes
- **WHEN** a frame is rendered with `pp_bloom_enabled` false
- **THEN** no bloom passes are added and the post-process pass samples only the scene input

#### Scenario: Shadow settings change at runtime
- **WHEN** a preset changes `sm_resolution` or `sm_cascade_count` while running
- **THEN** the shadow framebuffer is recreated with the new resolution/layer count before the next shadow
  pass


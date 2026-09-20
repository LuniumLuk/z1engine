## MODIFIED Requirements

### Requirement: Preset definitions

The presets SHALL map to the following render settings and SHALL NOT modify artistic values (e.g. AO radius,
exposure, gamma, sun parameters):

| Setting | LOW | MEDIUM | HIGH |
|---|---|---|---|
| `taa_enabled` / `taa_sharpen_enabled` | off / off | on / off | on / on |
| `ao_enabled` / `ao_resolution` / `ao_blur_enabled` | on / quarter / on | on / half / on | on / half / on |
| `pp_bloom_enabled` | off | on | on |
| `ssr_enabled` | off | unchanged | on |
| `sm_resolution` / `sm_cascade_count` | 1024 / 1 | 2048 / 2 | 2048 / 4 |
| `msaa_samples` | 2x | 2x | 4x |

#### Scenario: LOW disables expensive features

- **WHEN** LOW is applied
- **THEN** TAA (and its sharpen), bloom and SSR are disabled and the shadow map runs at 1024² with one
  cascade, while AO stays enabled at quarter resolution

#### Scenario: HIGH matches the previous defaults

- **WHEN** HIGH is applied
- **THEN** TAA with sharpen, half-resolution AO, bloom, and 2048² × 4 cascades shadows are enabled, matching
  the engine's previous default configuration

#### Scenario: HIGH enables screen-space reflections

- **WHEN** HIGH is applied
- **THEN** `ssr_enabled` is set to true so the deferred pipeline includes the SSR pass and the scene's
  authored `ssr_intensity` and ray-march parameters take effect

#### Scenario: LOW and MEDIUM set the MSAA level

- **WHEN** LOW or MEDIUM is applied
- **THEN** `msaa_samples` is set to 2x

#### Scenario: HIGH sets the MSAA level

- **WHEN** HIGH is applied
- **THEN** `msaa_samples` is set to 4x

#### Scenario: Artistic settings untouched

- **WHEN** any preset is applied
- **THEN** settings such as `ao_radius`, `ao_intensity`, `pp_exposure`, `pp_gamma` and sun parameters keep
  their scene-authored values

### Requirement: Preset toggles take effect in the render graph

Turning TAA off SHALL skip the velocity, TAA and TAA-sharpen passes and route the post-process pass directly
from the scene-color output. Turning bloom off SHALL skip the bloom downsample/upsample chain and the
post-process pass SHALL not depend on bloom outputs. Shadow settings SHALL be re-applied to renderer
resources when changed. Changing the MSAA level SHALL rebuild intermediate render targets at the new sample
count before the next frame, without an application restart.

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

#### Scenario: MSAA level change at runtime

- **WHEN** a preset changes `msaa_samples` while running
- **THEN** multisampled intermediate targets are created (or released) and reused/recreated at the new
  sample count before the next frame is rendered

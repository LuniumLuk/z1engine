# shadow-cascade-configuration Specification

## Purpose
TBD - created by archiving change add-probing-and-quality-presets. Update Purpose after archive.
## Requirements
### Requirement: Configurable shadow map resolution and cascade count

`GlobalSettings` SHALL provide two reflected settings for cascaded shadow maps:

- `sm_resolution`: the shadow map edge resolution, one of 512, 1024, 2048, 4096 (default 2048).
- `sm_cascade_count`: the number of cascades, 1 to 4 inclusive (default 4).

Both SHALL be serialized with scenes (name-based enum serialization), editable in the editor inspector under
the shadow group, and settable from scripts/game code.

#### Scenario: Defaults preserve previous behavior
- **WHEN** a scene without these fields is loaded
- **THEN** the shadow map defaults to 2048² with 4 cascades

#### Scenario: Editor exposure
- **WHEN** the editor settings panel shows the shadow group
- **THEN** resolution and cascade count appear as editable dropdowns

### Requirement: Renderer honors the configured cascades

The renderer SHALL allocate the shadow depth array with exactly `sm_cascade_count` layers at
`sm_resolution` × `sm_resolution`, run exactly one shadow pass per cascade, compute CSM split distances and
light matrices for the configured cascade count, and pass the same count to particle shadow passes. Shadow
resources SHALL be recreated only when resolution or cascade count changed.

#### Scenario: Cascade count reduced
- **WHEN** `sm_cascade_count` is 2
- **THEN** the shadow framebuffer has 2 layers and the render graph contains exactly two shadow passes
  (plus particle shadow passes for two cascades when particles cast shadows)

#### Scenario: Single cascade
- **WHEN** `sm_cascade_count` is 1
- **THEN** one shadow pass renders into layer 0 and every in-range receiver maps to cascade 0

#### Scenario: Resource recreation on change
- **WHEN** the configured resolution or cascade count differs from the currently allocated shadow resources
- **THEN** the framebuffer is recreated before the next shadow pass and the shadow image reference is updated

### Requirement: Shader cascade selection respects the cascade count

The lighting and particle shaders SHALL select the cascade from the `u_csm_cascade_count` uniform and the
`u_csm_splits` distances so that only existing layers are sampled: with one cascade every in-range distance
maps to layer 0; distances beyond the furthest split clamp to the last valid cascade; layers at or beyond the
count SHALL never be sampled. Unused `u_sun_projview` slots SHALL contain a valid matrix.

#### Scenario: Four cascades keep the classic mapping
- **WHEN** `cascade_count` is 4
- **THEN** the cascade index follows the three split distances as before, with the far split selecting the
  last cascade

#### Scenario: Fewer cascades never sample missing layers
- **WHEN** `cascade_count` is N < 4
- **THEN** the computed index is always < N for any world distance

#### Scenario: Beyond the shadow range
- **WHEN** a receiver distance exceeds the last cascade's far split
- **THEN** the last valid cascade is used and no out-of-range layer is sampled


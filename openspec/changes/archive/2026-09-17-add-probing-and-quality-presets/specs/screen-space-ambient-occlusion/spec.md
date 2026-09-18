## MODIFIED Requirements

### Requirement: AO generation pass

The renderer SHALL generate a screen-space ambient occlusion texture from the scene's depth and normal buffers, at half or quarter of the render resolution as selected by the global `ao_resolution` setting (half by default), before the lighting stage of the pipeline. Two algorithms SHALL be supported and selected by the global `ao_type` setting:

- **SSAO** (`ao_type = 0`): hemisphere sampling with a per-pixel rotated 16-sample kernel in view space, with distance range-check and depth bias.
- **GTAO** (`ao_type = 1`): Jimenez 2016 horizon-based AO with 6 slices × 8 steps, per-slice projected-normal weighting, cosine-weighted integration, and horizon clamping.

Pixels with no geometry (depth cleared, e.g. sky) SHALL produce AO = 1.0 (fully unoccluded).

#### Scenario: SSAO selected
- **WHEN** `ao_enabled` is true and `ao_type` is 0
- **THEN** the AO pass runs the SSAO shader and produces an AO texture with values in [0,1]

#### Scenario: GTAO selected
- **WHEN** `ao_enabled` is true and `ao_type` is 1
- **THEN** the AO pass runs the GTAO shader and produces an AO texture with values in [0,1]

#### Scenario: Quarter-resolution AO
- **WHEN** `ao_enabled` is true and `ao_resolution` selects quarter resolution
- **THEN** the AO and blur framebuffers are allocated at one quarter of the render width and height and the lighting stage samples the quarter-resolution result

#### Scenario: AO disabled
- **WHEN** `ao_enabled` is false
- **THEN** no AO or blur pass is added and lighting uses AO = 1.0 everywhere

#### Scenario: No geometry in pixel
- **WHEN** a pixel's depth equals the cleared depth value (no surface)
- **THEN** the pixel's AO is 1.0

# screen-space-ambient-occlusion Specification

## Purpose
TBD - created by archiving change add-ambient-occlusion. Update Purpose after archive.
## Requirements
### Requirement: AO generation pass

The renderer SHALL generate a screen-space ambient occlusion texture from the scene's depth and normal buffers, at half the render resolution, before the lighting stage of the pipeline. Two algorithms SHALL be supported and selected by the global `ao_type` setting:

- **SSAO** (`ao_type = 0`): hemisphere sampling with a per-pixel rotated 16-sample kernel in view space, with distance range-check and depth bias.
- **GTAO** (`ao_type = 1`): Jimenez 2016 horizon-based AO with 6 slices × 8 steps, per-slice projected-normal weighting, cosine-weighted integration, and horizon clamping.

Pixels with no geometry (depth cleared, e.g. sky) SHALL produce AO = 1.0 (fully unoccluded).

#### Scenario: SSAO selected
- **WHEN** `ao_enabled` is true and `ao_type` is 0
- **THEN** the AO pass runs the SSAO shader and produces an AO texture with values in [0,1]

#### Scenario: GTAO selected
- **WHEN** `ao_enabled` is true and `ao_type` is 1
- **THEN** the AO pass runs the GTAO shader and produces an AO texture with values in [0,1]

#### Scenario: AO disabled
- **WHEN** `ao_enabled` is false
- **THEN** no AO or blur pass is added and lighting uses AO = 1.0 everywhere

#### Scenario: No geometry in pixel
- **WHEN** a pixel's depth equals the cleared depth value (no surface)
- **THEN** the pixel's AO is 1.0

### Requirement: AO blur pass

When `ao_blur_enabled` is true, the raw AO texture SHALL be filtered by an edge-preserving blur pass that weights samples by a Gaussian kernel and reduces weight at depth discontinuities, using the scene depth buffer. The blurred result SHALL be the AO texture consumed by lighting.

#### Scenario: Blur enabled
- **WHEN** `ao_blur_enabled` is true
- **THEN** the lighting stage samples the blurred AO texture

#### Scenario: Blur disabled
- **WHEN** `ao_blur_enabled` is false
- **THEN** the lighting stage samples the raw (unblurred) AO texture

### Requirement: AO applied to ambient lighting

Both the deferred and forward pipelines SHALL multiply the ambient (indirect) lighting term by the AO factor sampled from the AO texture. Direct light contributions (sun, point, spot) and specular SHALL NOT be multiplied by the screen-space AO factor.

#### Scenario: Deferred pipeline applies AO
- **WHEN** a frame is rendered with the deferred renderer and AO enabled
- **THEN** the deferred lighting pass multiplies `ambient = base_color * u_sun_ambient.rgb` by the sampled AO factor

#### Scenario: Forward pipeline applies AO
- **WHEN** a frame is rendered with the forward renderer and AO enabled
- **THEN** the forward PBR/phone lighting multiplies the ambient term by the sampled AO factor

#### Scenario: AO does not affect direct light
- **WHEN** a pixel has AO = 0.5
- **THEN** its direct sun/point/spot contributions remain unchanged while ambient is halved

### Requirement: Forward depth+normal prepass

The forward pipeline SHALL render opaque and masked geometry to a depth+normal prepass before the AO pass when AO is enabled, so the AO pass has a complete depth and normal buffer before the main forward lighting pass. The prepass SHALL be skipped when `ao_enabled` is false.

#### Scenario: Forward prepass runs
- **WHEN** the forward renderer renders with AO enabled
- **THEN** a prepass renders opaque+mask geometry to depth and normal targets before the AO pass

#### Scenario: Forward prepass skipped
- **WHEN** the forward renderer renders with AO disabled
- **THEN** no prepass is added

### Requirement: AO settings exposure

The global settings SHALL expose `ao_enabled`, `ao_type` (0 = SSAO, 1 = GTAO), `ao_radius`, `ao_intensity`, `ao_power`, `ao_bias`, `ao_blur_enabled`, and `ao_blur_strength` through the reflection-driven editor inspector, and SHALL propagate them to shaders through the global uniform buffer.

#### Scenario: Settings reach the shader
- **WHEN** an AO parameter is changed in the editor
- **THEN** the AO pass uses the updated value on the next frame

### Requirement: AO texture binding in forward materials

Forward material shaders SHALL receive the AO texture through the engine's per-frame binding mechanism (`PerFrameConst::ao_map_binding`), and SHALL only sample it when the global `ao_enabled` flag is set, so unbound sampler uniforms are never sampled.

#### Scenario: Material samples AO
- **WHEN** a forward material fragment is shaded with AO enabled
- **THEN** it samples the AO texture at the fragment's screen UV and multiplies the ambient term

#### Scenario: Material skips AO when disabled
- **WHEN** a forward material fragment is shaded with AO disabled
- **THEN** it does not sample the AO texture and the ambient term is unmodified


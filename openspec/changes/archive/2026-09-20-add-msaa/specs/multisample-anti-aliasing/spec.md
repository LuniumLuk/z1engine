## ADDED Requirements

### Requirement: MSAA setting and hardware clamping

`GlobalSettings` SHALL expose a reflected `msaa_samples` setting with values Off (1), 2x (2), 4x (4) and
8x (8), defaulting to Off. The renderers SHALL use an effective sample count of
`min(requested, GL_MAX_SAMPLES, maximum samples supported for the attachment formats)`. If the effective
count is below 2, MSAA SHALL be disabled for that frame and a single `CORE_WARN` SHALL be logged. Changing
the setting SHALL take effect on the next frame without restarting the application, and `msaa_samples = Off`
SHALL produce the same render graph, resource set and behavior as before the feature existed.

#### Scenario: MSAA off renders exactly the legacy graph

- **WHEN** a frame renders with `msaa_samples = Off`
- **THEN** no multisampled attachments, resolve passes or edge passes are created
- **AND** the pass graph is identical to the non-MSAA pipeline

#### Scenario: Requested sample count is honored

- **WHEN** `msaa_samples = X4` and the driver supports 4 samples for all attachment formats
- **THEN** the G-buffer (or forward main pass) targets are created with 4 samples

#### Scenario: Requested count exceeds hardware capability

- **WHEN** the requested count exceeds the driver's maximum for the attachment formats
- **THEN** the renderers use the clamped maximum
- **AND** a warning is logged once

#### Scenario: MSAA unsupported degrades gracefully

- **WHEN** the effective sample count would be below 2
- **THEN** MSAA is disabled for the frame (single-sample graph) and a `CORE_WARN` is logged
- **AND** rendering continues correctly

#### Scenario: Setting change applies at runtime

- **WHEN** the setting changes from Off to a multisampled value (or back) while the editor is running
- **THEN** the next frame uses the new sample count
- **AND** intermediate framebuffers are recreated only when their sample count actually changed

### Requirement: Deferred MSAA G-buffer with resolve to single-sample resources

With MSAA enabled, the deferred pipeline SHALL render the G-buffer (5 color attachments + depth) into
multisampled textures, SHALL produce single-sample copies of all six G-buffer resources through a resolve
pass, and SHALL keep the screen-space consumers (AO, SSR, velocity, bulk lighting) reading those
single-sample resources under their existing names. The deferred lighting pass SHALL write a multisampled
scene color, and a resolve SHALL produce the single-sample scene color consumed by SSR, TAA, bloom,
post-processing and the deferred transparency/particle passes.

#### Scenario: G-buffer rendered multisampled

- **WHEN** a deferred frame renders with MSAA on
- **THEN** the gbuffer pass targets are multisampled and a resolve pass produces the single-sample
  `gbuffer-position`, `gbuffer-normal`, `gbuffer-albedo`, `gbuffer-metallic-roughness`, `gbuffer-emissive`
  and `gbuffer-depth` resources

#### Scenario: Screen-space stages see single-sample inputs

- **WHEN** AO, SSR or velocity passes run in an MSAA frame
- **THEN** they sample the resolved single-sample G-buffer with regular samplers and keep their non-MSAA
  semantics

#### Scenario: Scene color resolved before the temporal chain

- **WHEN** an MSAA frame reaches SSR, TAA, bloom or post-processing
- **THEN** they consume the resolved single-sample scene color, never a multisampled texture

#### Scenario: Blended transparency composites after resolve

- **WHEN** blended geometry renders in the deferred pipeline with MSAA on
- **THEN** it loads on top of the resolved scene color (documented limitation: blended objects have no MSAA
  coverage in this pipeline, while opaque and masked geometry do)

### Requirement: Sample-frequency edge shading in deferred lighting

Deferred lighting SHALL be implemented as two passes sharing one multisampled scene-color target: a bulk
pass that shades once per pixel (without referencing `gl_SampleID`) and an edge pass that runs at sample
frequency and discards every fragment whose pixel is fully covered by one surface. A pixel SHALL be
classified as an edge when any sample's G-buffer depth differs from sample 0's depth. Edge pixels SHALL be
shaded per sample using that sample's own G-buffer attributes
(`texelFetch(sampler2DMS, pixel, gl_SampleID)`), applying the G-buffer sky convention (alpha == 0 →
emissive/background) per sample, and each sample's result SHALL be written with
`gl_SampleMask[0] = 1 << gl_SampleID`. The edge path SHALL be declared as a shader variant
(`VARIANT_MSAA_EDGE`) so the shader validator compiles every declared variant combination.

#### Scenario: Silhouette against the sky is anti-aliased

- **WHEN** an object silhouette partially covers a pixel over the sky in an MSAA frame
- **THEN** covered samples show the object and uncovered samples show the sky background, producing
  intermediate edge colors after the resolve

#### Scenario: Silhouette against other geometry is anti-aliased

- **WHEN** a foreground object partially covers a pixel overlapping a distant surface
- **THEN** each sample is shaded with its own geometry's attributes

#### Scenario: Fully covered pixels are not re-shaded

- **WHEN** all samples of a pixel share the same depth
- **THEN** the edge pass discards that fragment and the bulk lighting result stands

#### Scenario: Bulk lighting program stays per-pixel

- **WHEN** the bulk lighting program is inspected
- **THEN** it does not reference `gl_SampleID` or `gl_SampleMask`, so it is not executed at sample frequency

#### Scenario: Shader variants validate

- **WHEN** `validate-shaders` runs
- **THEN** `deferred_lighting.glsl` compiles and links both with and without `VARIANT_MSAA_EDGE`

### Requirement: Forward MSAA main pass

With MSAA enabled, the forward pipeline SHALL render its main pass (opaque, masked, blended geometry and the
skybox) into multisampled color and depth targets and SHALL resolve them into the single-sample
`scene-color` and `scene-depth` resources before the velocity/TAA/bloom/post-processing chain. No additional
shader work SHALL be required for forward MSAA (coverage anti-aliasing happens at rasterization).

#### Scenario: Forward frame with MSAA on

- **WHEN** the forward renderer draws with MSAA on
- **THEN** the main pass targets are multisampled and a resolve pass produces single-sample scene color and
  depth for the remaining chain

#### Scenario: Blended geometry gets coverage AA in forward mode

- **WHEN** blended geometry renders in the forward main pass with MSAA on
- **THEN** its silhouette is anti-aliased by sample coverage like any other geometry

### Requirement: MSAA composes with TAA and post-processing

MSAA and TAA SHALL be independently toggleable and MAY both be enabled; the resolve SHALL run before the
velocity/TAA stages. No multisampled texture SHALL ever be bound to a regular sampler slot; multisampled
sampler slots SHALL always receive multisampled textures (or the type-matched fallback).

#### Scenario: TAA and MSAA enabled together

- **WHEN** a frame renders with both TAA and MSAA enabled
- **THEN** TAA consumes the resolved scene color and the velocity pass consumes the resolved depth, with no
  multisampled texture bound to a `sampler2D` slot

#### Scenario: MSAA provides AA without TAA

- **WHEN** TAA is disabled and MSAA is enabled (e.g. LOW preset)
- **THEN** geometry silhouettes are anti-aliased without temporal accumulation

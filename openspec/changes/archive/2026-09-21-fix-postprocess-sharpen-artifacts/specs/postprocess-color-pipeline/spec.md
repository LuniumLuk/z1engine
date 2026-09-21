## ADDED Requirements

### Requirement: Post-process pass composition

The final post-process pass (`postprocessing.glsl`) SHALL apply only non-spatial color operations, in this
order: bloom composite, negative clamp, exposure, Reinhard tone mapping, tint, gamma correction. It SHALL
NOT apply any neighbourhood (spatial) filter, sharpen or unsharp mask, because such filters can drive
radiance negative and thereby saturate or invert through the tone curve; any such filter additionally
imprints its own kernel footprint (e.g. a 5-pixel cross for a center + 4 axial-neighbour kernel) and
amplifies single-pixel shading signals into saturated white/black pixels.

#### Scenario: No spatial filtering in the post-process pass

- **WHEN** a frame reaches the post-process pass in either the deferred or the forward renderer
- **THEN** the pass samples the scene color once per pixel at `v_uv` and combines it with the bloom texture,
  exposure, tone mapping, tint and gamma only
- **AND** no `up`/`down`/`left`/`right` (or other offset) scene-color samples are taken

#### Scenario: Non-negative input to tone mapping

- **WHEN** the post-process pass assembles the HDR color (scene color with optional bloom composite)
- **THEN** the color is clamped to a minimum of zero before exposure, tone mapping, tint and gamma, so the
  non-monotonic Reinhard curve (pole at -1) and the gamma `pow()` never receive negative values and cannot
  produce saturated-white or NaN-black pixels from negative radiance

#### Scenario: Sharpening is exclusively the gated TAA sharpen pass

- **WHEN** `taa_sharpen_enabled` is false, or TAA is disabled
- **THEN** no sharpening is applied anywhere in the frame
- **AND** when `taa_enabled && taa_sharpen_enabled` are true, sharpening is applied only by the dedicated
  `taa_sharpen.glsl` pass with `taa_sharpen_strength`

#### Scenario: Smooth surfaces render without cross artifacts

- **WHEN** a glossy surface with an environment-map reflection is rendered with any `msaa_samples` value
  (Off/2x/4x/8x)
- **THEN** the final image contains no isolated saturated-white or saturated-black pixels arranged as
  5-pixel crosses, and no salt-and-pepper amplification of specular sparkle beyond the pre-existing shading
  signal

#### Scenario: Post-process pass is unchanged otherwise

- **WHEN** the post-process pass runs with bloom enabled or disabled
- **THEN** the existing bloom composite, exposure, tone mapping, tint and gamma behavior is preserved

## MODIFIED Requirements

### Requirement: AO texture binding in forward materials

Forward material shaders SHALL receive the AO texture through the engine's per-frame binding mechanism (`PerFrameConst::ao_map`), which the material binding plan resolves to the `u_ao_texture` sampler slot at bind time, and SHALL only sample it when the global `ao_enabled` flag is set, so unbound sampler uniforms are never sampled.

#### Scenario: Material samples AO
- **WHEN** a forward material fragment is shaded with AO enabled
- **THEN** it samples the AO texture at the fragment's screen UV and multiplies the ambient term

#### Scenario: Material skips AO when disabled
- **WHEN** a forward material fragment is shaded with AO disabled
- **THEN** it does not sample the AO texture and the ambient term is unmodified

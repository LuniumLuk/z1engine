## 1. Component & Serialisation

- [x] 1.1 Add `bool m_receive_shadows = true` field to `ParticleComponent` in `engine/runtime/source/scene/component/particle.h`
- [x] 1.2 Serialise `m_receive_shadows` in the scene save/load path; ensure missing field deserialises as `true`
- [x] 1.3 Expose `m_receive_shadows` toggle in the editor particle inspector panel
- [x] 1.4 Add `bool m_cast_shadows = true` field to `ParticleComponent` with serialisation and editor exposure

## 2. Shader

- [x] 2.1 Add CSM shadow-sampling uniforms to `particle.glsl`: `u_shadow_map` (sampler2DArray), `u_sun_projview[4]` (mat4), `u_csm_splits` (vec4), `u_receive_shadows` (int)
- [x] 2.2 Implement cascade-selection logic in `particle.glsl` matching `deferred_lighting.glsl`
- [x] 2.3 Implement 3×3 PCF shadow factor with fixed depth-bias
- [x] 2.4 Multiply shadow factor into final fragment colour when `u_receive_shadows == 1`
- [x] 2.5 Validate shader compiles without errors: `python dev/z1.py validate-shaders`
- [x] 2.6 Create `particle_shadow.glsl` — depth-only billboard shader using `u_sun_projview[u_csm_index]`, alpha discard

## 3. Renderer Binding

- [x] 3.1 In `particle_renderer.cpp`, bind shadow image and set Global UBO block binding explicitly
- [x] 3.2 Bind shadow map texture array and set `u_receive_shadows` uniform per emitter
- [x] 3.3 Set `u_receive_shadows` uniform to `1` or `0` based on `ParticleComponent::m_receive_shadows`
- [x] 3.4 Ensure shadow resources are not bound when no directional light casts shadows
- [x] 3.5 Add `m_pipeline_shadow` pipeline and `add_particle_shadow_passes` to `ParticleRenderer`
- [x] 3.6 Implement shadow passes using `LoadOp::Load` (preserve mesh shadows), one pass per CSM cascade
- [x] 3.7 Call `add_particle_shadow_passes` after `add_shadow_pass` in deferred and forward renderers

## 4. Validation

- [x] 4.1 Run full shader validation: `python dev/z1.py validate-shaders`
- [x] 4.2 Build the project: `python dev/z1.py compile`
- [x] 4.3 Smoke-test: `python dev/z1.py smoke`
- [ ] 4.4 Manually verify a particle emitter in a shadowed region darkens correctly
- [ ] 4.5 Manually verify particles cast shadows onto geometry below them
- [ ] 4.6 Verify `m_receive_shadows = false` and `m_cast_shadows = false` per-emitter controls work

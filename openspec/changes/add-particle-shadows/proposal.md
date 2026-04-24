## Why

Particles currently ignore shadow maps, so emitters sitting inside shadowed areas appear fully lit — breaking visual coherence with the rest of the scene. Adding shadow reception to particles makes effects like smoke, dust, and debris feel grounded in the environment's lighting.

## What Changes

- Particle billboards sample the existing CSM shadow map during rendering, darkening particles that fall inside shadowed regions
- `ParticleComponent` gains a `m_receive_shadows` flag (default `true`) so authors can opt individual emitters out of shadow sampling for cost-sensitive effects
- `particle.glsl` is extended with CSM shadow-sampling logic matching the technique used by opaque surfaces
- The particle renderer binds the shadow map and CSM uniforms when shadow reception is enabled

## Capabilities

### New Capabilities

- `particle-shadow-receive`: Particles sample the directional-light CSM shadow map; a per-emitter flag controls opt-out; attenuation matches the shadow factor used by opaque meshes

### Modified Capabilities

- `particle-system-robustness`: `ParticleComponent` gains the `m_receive_shadows` field; serialisation and default value must be covered by the existing robustness requirements

## Impact

- `engine/runtime/source/scene/component/particle.h` — add `m_receive_shadows` flag
- `engine/runtime/source/render/renderer/particle_renderer.h/.cpp` — bind shadow map and CSM uniforms, pass shadow flag to shader
- `engine/content/shader/particle.glsl` — implement CSM shadow factor, multiply into final colour
- Editor particle inspector — expose `m_receive_shadows` toggle

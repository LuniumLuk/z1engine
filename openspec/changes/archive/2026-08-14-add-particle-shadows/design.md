## Context

Particles render in a dedicated billboard pass after the main scene geometry. The render graph already produces a CSM shadow depth array (4 cascades, 2048×2048) consumed by the deferred lighting and forward passes. The particle renderer binds the scene depth texture for soft-particle blending but has no access to the shadow map today.

The particle shader (`particle.glsl`) handles texture sampling, alpha blending, and soft-particle fade; it does not perform any lighting computation.

## Goals / Non-Goals

**Goals:**
- Particles sample the directional-light CSM shadow map and are attenuated in shadowed regions
- A per-emitter boolean `m_receive_shadows` (default `true`) lets authors disable shadow sampling for cost-sensitive effects
- Shadow attenuation uses the same cascade-selection and PCF logic already used by opaque surfaces, so particle shadows match scene shadow quality

**Non-Goals:**
- Particles casting shadows onto scene geometry — alpha-blended billboards produce noisy depth maps; this is deferred to a future change
- Point-light or spot-light shadow reception — only the directional CSM is targeted
- Per-particle shadow intensity control beyond the emitter-level flag

## Decisions

### Re-use existing CSM resources

**Decision**: Bind `m_shadow_image` and the `sun_projview`/`csm_splits` uniforms that already exist in `RenderShared` to the particle pass.

**Rationale**: The shadow map is already computed and resident in GPU memory before the particle pass executes. Duplicating it or introducing a separate shadow pass for particles would waste memory bandwidth and complicate the render graph dependency chain.

**Alternative considered**: Render particles to a separate low-res shadow mask texture, then composite. Rejected — adds a full extra render pass and a texture lookup per fragment with no quality benefit.

---

### Shadow receive only; no shadow cast

**Decision**: Particles receive shadows from the scene directional light but do not cast shadows onto scene geometry or other particles.

**Rationale**: Alpha-blended billboards produce severe shadow acne and light-bleed artefacts in depth passes without alpha-tested pre-passes. The engineering cost outweighs the visual benefit for typical particle effects (smoke, sparks, dust). Shadow casting can be revisited as a separate opt-in feature.

---

### Per-emitter opt-out flag

**Decision**: `ParticleComponent::m_receive_shadows` (bool, default `true`). When `false`, the particle pass skips shadow uniform binding and uses a shader variant without the shadow sampling branch.

**Rationale**: Additive emitters (fire, magic glows) look worse with shadow attenuation because they are supposed to emit light, not receive it. Authors need a cheap escape hatch. A single bool is cheaper than a per-blend-mode heuristic and keeps the system explicit.

**Alternative considered**: Auto-disable shadow reception for `Additive` blend mode. Rejected — couples rendering policy to blend mode enum and removes author control.

---

### Cascade selection matches opaque surfaces

**Decision**: Use the same `csm_splits` comparison and `sun_projview` transform used in `deferred_lighting.glsl` and `pbr.glsl`.

**Rationale**: Consistent cascade selection avoids visible shadow boundary mismatches between opaque and particle surfaces in the same frame.

---

### PCF sampling

**Decision**: Apply the same 3×3 PCF kernel used by opaque surfaces (no extra softening for particles).

**Rationale**: Consistency. A softer kernel would differ visually from scene shadows and require tweaking. Particles are already soft-blended at edges via soft-particle depth fade, which compensates perceptually.

## Risks / Trade-offs

- **[Risk] Performance** — Every alive particle fragment samples the shadow map with a 3×3 PCF kernel (9 texture taps). High-particle-count scenes may observe a measurable GPU cost increase.  
  → **Mitigation**: The `m_receive_shadows` flag provides a per-emitter escape hatch. Artists can disable it for high-density emitters after profiling.

- **[Risk] Shadow acne on particles** — Billboards rotate to face the camera; their world-space depth relationship to the shadow cascade is unusual and may produce self-shadowing at grazing cascade boundaries.  
  → **Mitigation**: Apply the same constant depth bias already used for opaque surfaces. If artefacts are pronounced, a small slope-scale bias can be added as a follow-up.

- **[Risk] Cascade edge banding** — Particles near CSM split planes may flicker between cascades as they move.  
  → **Mitigation**: Standard cascade blending (already in the lighting shader) will be replicated in the particle shader.

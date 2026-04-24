## Context

The engine has two render pipelines — forward and deferred — that share the same scene graph but resolve lighting differently. When a mesh primitive has no assigned material, both renderers fall back to `m_default_material` (the `MI_phone` material instance). In the forward path the `phone.glsl` fragment shader calls `phone_shading()` (Phong model); in the deferred path `gbuffer_out_phone.glsl` writes PBR G-buffer values and `deferred_lighting.glsl` calls `calculate_pbr_illumination()` (Cook-Torrance). The two functions produce different energy at the same illumination level: Phong's diffuse term is `NoL * intensity`, while Cook-Torrance's is `(base / π) * radiance * NoL`, making forward output roughly 3× brighter.

## Goals / Non-Goals

**Goals:**
- `phone.glsl`'s forward fragment stage produces the same luminance as the deferred pipeline for the same scene.
- The forward path uses the same metallic / roughness constants (0.0 / 0.5) as `gbuffer_out_phone.glsl`.
- No visual change to materials that already have an explicit PBR material assigned.

**Non-Goals:**
- Changing the deferred lighting pipeline or G-buffer layout.
- Making `phone.glsl` a fully editable PBR material (roughness / metallic remain hardcoded constants matching the G-buffer approximation).
- Removing `phone_shading()` from `lighting.glsl` if other callers exist (audit first; remove only if safe).

## Decisions

### Use `calculate_pbr_illumination` in the forward phone path

**Decision**: Replace the `phone_shading(...)` call in `phone.glsl`'s `else` (forward, non-G-buffer, non-shadow, non-velocity) branch with the same `calculate_pbr_illumination` loop already used by `deferred_lighting.glsl`, using hardcoded metallic = 0.0 and roughness = 0.5.

**Rationale**: The deferred path already handles ambient + sun + dynamic lights via `calculate_pbr_illumination`. Reusing it in the forward path costs near-zero code change and guarantees mathematical parity. A Phong→PBR re-calibration (e.g. adjusting a scale factor) was considered but rejected because it would still diverge under colored lights.

**Alternative rejected**: Keep Phong, add a `/ PI` correction factor. Rejected because specular energy conservation also differs between models, so diffuse-only correction would leave specular divergence.

### Keep `phone_shading` in `lighting.glsl`

**Decision**: Do not delete `phone_shading`. Other shaders (e.g. `lambert_shading`) co-exist in `lighting.glsl`; removing `phone_shading` risks breaking future or external users. Leave it in place; the fix only stops calling it from `phone.glsl`.

## Risks / Trade-offs

| Risk | Mitigation |
|------|-----------|
| Forward scene becomes visibly darker after change (especially brightly lit test levels) | Expected and correct — deferred was always the reference. Document as intentional in PR. |
| `phone_shading` dead-code confusion | Add a comment that it is superseded by `calculate_pbr_illumination` for the default material path. |
| Ambient formula mismatch | Forward `phone_shading` uses `color.rgb * u_sun_ambient.rgb`; `deferred_lighting.glsl` uses the same formula (`base_color * u_sun_ambient.rgb`). No change needed. |

## Migration Plan

1. Edit `phone.glsl` forward fragment branch to replicate the PBR lighting loop from `deferred_lighting.glsl`.
2. Validate shaders (`python dev/z1.py validate-shaders`).
3. Smoke-test both pipelines with the same scene containing meshes with and without explicit materials.
4. Rollback: revert the single shader edit; no data or API changes involved.

## Open Questions

- None. The fix is self-contained to a single shader file.

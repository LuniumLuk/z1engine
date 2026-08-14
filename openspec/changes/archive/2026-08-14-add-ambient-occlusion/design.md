## Context

The engine has two render paths sharing `RenderShared` infrastructure:

- **Deferred** (`renderer_deferred.cpp`): G-buffer (position RGBA32F, normal RGB16F, albedo, metallic-roughness, emissive, depth) → shadow → deferred lighting → forward transparency → velocity → TAA → bloom → post.
- **Forward** (`renderer_forward.cpp`): shadow → main (opaque + blend + skybox in one pass) → particles → velocity → TAA → bloom → post.

Both write a constant `sun_ambient` term in lighting and have no contact occlusion. Global UBO (`GlobalSettings`) is the standard channel for renderer-wide toggles and already carries TAA/bloom/shadow tunables exposed in the editor via `REFLECTED_FIELD`. Shaders are GLSL in `engine/content/shader/` with a `@uniforms`/`@reflections`/`@variants` system and shared includes (`uniforms.glsl`, `vert.glsl`, `frag_attrs.glsl`, `lighting.glsl`). Fullscreen passes use `quad.glsl` and the `m_quad` VAO in `RenderShared`.

The GTAO math is verified from three independent reference implementations (rin-miku GTAO, FengRender, arycama/EveryRay) and documented in the personal wiki (`D:\wiki\wiki\concepts\z1engine-gtao-ssao.md`).

## Goals / Non-Goals

**Goals:**
- AO term available in both deferred and forward pipelines.
- Two selectable algorithms: classic SSAO and Jimenez GTAO.
- Half-resolution AO with optional edge-preserving blur for cost control.
- Tunables exposed in the editor inspector and driven through the global UBO.
- No new third-party dependencies.

**Non-Goals:**
- Temporal reprojection of AO (engine TAA already denoises; blur + half-res suffice).
- Bent normals / ambient occlusion on particles or 2D sprites.
- AO on transparent (blended) surfaces beyond ambient sampling of the AO texture.
- Reconstructing normals from depth — both pipelines already have a normal buffer available.

## Decisions

### D1. AO computed in view space from depth + normal, with position reconstructed from depth
AO shaders take a (non-linear) depth texture, a world-space normal texture, `u_proj`, `u_inv_proj`, `u_view`. View-space position is reconstructed per pixel from depth via `u_inv_proj`; normals are transformed to view space with `mat3(u_view)`. This keeps the AO shaders identical for deferred (reads G-buffer depth/normal) and forward (reads prepass depth/normal).

- *Alternative considered:* reconstructing from the G-buffer world-position texture. Rejected: forward prepass would then need to store position, and the reconstruction path is uniform across both pipelines anyway.

### D2. GTAO per-slice formula (verified)
Per slice: project the normal onto the slice plane (`w = |projN|` slice weight, signed angle `n` from the view vector), march ±the slice direction in screen space (steps converted from world `ao_radius` via `radius_uv = radius / (proj[1][1]·|P.z|)`), track horizon cosines with distance-falloff + thin-occluder blending, clamp horizons to ±π/2 around `n`, and accumulate `0.25·(cosN + 2h·sin n − cos(2h−n))` per horizon, weighted by `w`, then normalize by Σw. Multi-bounce approximation folded into `u_power` (applied as `pow`) rather than the albedo polynomial (we lack per-pixel albedo at AO time in the forward prepass; `pow` is a close, cheap substitute).

### D3. Forward pipeline gets a depth+normal prepass that reuses the GBuffer shader variant
Forward AO needs depth+normals before lighting, which a single-pass forward renderer cannot produce. A `prepass` pass renders opaque+mask geometry with `variant_key = ShaderVariant::GBuffer` into a framebuffer whose attachments match the shader's output locations 0–1 (position RGBA32F, normal RGB16F) plus depth; outputs 2–4 (albedo/MR/emissive) are dropped by OpenGL since no matching attachments exist. No new shader variant is required.

- *Alternative considered:* a dedicated `VARIANT_PREPASS` outputting only normal+depth. Rejected: adds a variant to every forward material shader and a new include; the GBuffer reuse is simpler and the extra dropped outputs are free.

### D4. AO texture bound to forward materials via `PerFrameConst::ao_map_binding`
`MaterialInstance::bind` already binds `u_shadow_map` when `per_frame.shadow_map_binding` is valid. The same pattern is added for `u_ao_texture` (declared in shared `uniforms.glsl`). Forward fragment shaders sample it with a new `v_screen_uv` varying (from `vert.glsl`, location 8) guarded by the global `u_ao_enabled` flag, so unbound samplers are never sampled when AO is off.

- *Alternative considered:* per-material uniform/reflection plumbing. Rejected: this is an engine-owned texture, so the per-frame binding path is correct and keeps materials unchanged.

### D5. AO buffers are `RenderShared`-owned half-res framebuffers
`ensure_buffers()` sizes `m_ao_framebuffer` (raw) and `m_ao_blur_framebuffer` (blurred) at half resolution (RGBA8, `.r` = AO). `add_ao_pass(depth_input, normal_input)` adds an `"ao"` pass (+ `"ao-blur"` when enabled) writing to those framebuffers and returns the final pass name. Renderers bind `get_ao_image()` in the consuming passes and add an explicit `depends_on(ao_pass)` so ordering is guaranteed even though the AO buffer is bound directly rather than through `add_input`.

### D6. AO applied to the ambient (indirect) term only
Both `deferred_lighting.glsl` and the forward PBR/phone paths compute `ambient = base_color * u_sun_ambient.rgb`; the AO factor multiplies only that term. Direct sun/point/spot light and specular stay unoccluded, which is physically correct (AO describes indirect visibility) and avoids over-darkening.

## Risks / Trade-offs

- [Forward pipeline renders geometry twice (prepass + main) when AO is enabled] → Mitigation: prepass is depth+normal only and skipped entirely when `ao_enabled == 0`; half-res AO keeps the extra cost bounded.
- [Half-res AO with 8-bit RGBA8 can show banding] → Mitigation: `u_power` contrast shaping + optional blur; RGBA8 keeps bandwidth low.
- [GTAO cost at half-res (6 slices × 8 steps × 2 sides) may still be heavy on low-end GPUs] → Mitigation: SSAO (16 samples) is available as the cheaper preset; `ao_enabled` off disables the passes.
- [Depth buffer is non-linear; bilinear sampling could bleed across edges] → Mitigation: AO shaders use `texelFetch` for depth/normal reads; the blur uses depth-reconstructed view depth for edge stopping.
- [TAA jitter makes `v_screen_uv` sub-pixel-off for AO sampling] → Negligible: AO is half-res and blurred.

## Migration Plan

No serialization format changes; new `GlobalSettings` fields default to sane values (`ao_enabled = true`, `ao_type = GTAO`) and flow through the existing editor inspector. Rollback: set `ao_enabled = false` or revert the change; no asset migrations required.

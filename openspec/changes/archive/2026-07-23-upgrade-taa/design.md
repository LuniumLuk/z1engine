## Context

The engine uses a deferred-first render pipeline (with forward as an alternative). Both renderers share post-processing passes via `RenderShared`. The current TAA pipeline is:

```
Frame N:   Jitter proj → Render scene (jittered) → Velocity pass → TAA resolve → Bloom → Postprocess
Frame N+1: Jitter proj → Render scene (jittered) → Velocity pass → TAA resolve → ...
```

**Current TAA shader** (`taa.glsl`):
- Converts current and history colors to YCoCg
- Builds a 3×3 neighborhood AABB in YCoCg from current frame
- Reprojects history via `prev_uv = v_uv - velocity.xy`
- Clips history YCoCg to the AABB
- Blends: `mix(current_rgb, history_clipped_rgb, blend)` where `blend = u_taa_blend * velocity_mask`
- `velocity_mask` is always 1.0 (velocity shader outputs `.a = 1.0`)

**Jitter** (`renderer_deferred.cpp` / `renderer_forward.cpp`):
- Halton sequence (base 2, 3), offset by -0.5 to center around 0
- Converted to NDC: `ndc_x = 2.0 * jx / width`, `ndc_y = -2.0 * jy / height`
- Applied to projection matrix: `proj[2][0] += ndc_x`, `proj[2][1] += ndc_y`

**Velocity** (`velocity_out.glsl`):
- Computed as `curr_uv - prev_uv` where each is NDC→UV converted from clip-space positions
- `curr_clip` uses the jittered projview; `prev_clip` uses the previous frame's (also jittered) projview
- No explicit jitter information is encoded — the jitter is baked into the clip positions

**Key constraint**: OpenGL 4.6, GLSL 4.60, single compute-capable GPU (no async compute assumption).

## Root Cause Analysis

### R1: Jitter offset missing in TAA resolve (PRIMARY)

The Halton jitter offsets the projection matrix, which shifts the entire rendered image by ±0.5 pixels each frame. The TAA resolve shader reads `u_current_color` at `v_uv` (pixel center), but the color stored at that UV was actually rendered at `v_uv + jitter_offset_uv` in the current frame and at a completely different offset in the previous frame's history buffer.

The velocity already encodes the frame-to-frame motion of world points, and the jitter offset is implicitly included in the velocity because `curr_clip` and `prev_clip` both use jittered matrices. However, the **color sampling positions** in the TAA resolve do not account for where the jitter placed the sample within the pixel.

**Fix**: Pass the current frame's jitter offset (in UV units: `vec2(jx/width, jy/height)`) to the TAA shader. When sampling `u_current_color`, offset by `-jitter_uv` to read the value that would have been at pixel center. Alternatively, use a 2×2 gather or bicubic filter to reconstruct the center value.

### R2: Fixed high blend factor causes persistent error

`taa_blend = 0.9` means only ~10% of the current frame contributes per frame. The effective accumulation window is ~10 frames. Any error in the history (clipping artifacts, disocclusion, reprojection error) persists for many frames, creating visible ghosting/drift.

**Fix**: Compute per-pixel blend factor from local color statistics. The blend should be lower (more current frame) when:
- Local variance is high (edges, disocclusion) → reduce history weight
- Reprojected sample is far from neighborhood centroid → reduce history weight
- The reprojected UV falls outside [0,1] → reject history entirely

Reference formula (UE4-style):
```
blend = base_blend * (1.0 / (1.0 + variance_scale * local_variance))
blend = clamp(blend, min_blend, max_blend)
```

### R3: 3×3 YCoCg AABB is too restrictive

A 3×3 neighborhood cannot capture the full color variation of a textured surface with high-frequency detail. The clip rejects valid history that differs due to sub-pixel detail variation rather than true disocclusion. This causes flicker on detailed surfaces and prevents effective temporal accumulation.

**Fix**: Expand to a 5×5 or larger neighborhood, and use variance-guided clip expansion:
```
box_min = mean - gamma * sqrt(variance)
box_max = mean + gamma * sqrt(variance)
```
Where `gamma` (typically 1.0–2.0) controls how much the box is expanded beyond the observed range. This is the standard technique from Karis (UE4) and Salvi (AGRD).

### R4: No sharpening pass

TAA inherently acts as a low-pass filter. Without sharpening, the softened output appears to "swim" — high-frequency detail shifts slightly each frame as the jitter pattern changes, and the lack of sharp edges makes this motion perceptible.

**Fix**: Add a contrast-adaptive sharpening pass after TAA. Options:
- AMD FidelityFX CAS (Contrast Adaptive Sharpening) — well-tested, open-source, lightweight
- Simple unsharp mask with edge-stop (luma-guided) — easier to implement, sufficient quality

## Goals / Non-Goals

**Goals:**
- Eliminate visible frame-to-frame shakiness in stationary scenes
- Reduce ghosting on moving objects
- Maintain or improve anti-aliasing quality on edges
- Keep performance within 1–2ms GPU budget for TAA+sharpen combined
- Preserve API compatibility (settings, Python bindings) where possible

**Non-Goals:**
- DLSS/FSR-style temporal upscaling — resolution is fixed
- Motion blur — separate feature
- Ray-traced denoising integration — not applicable
- Multi-view (VR) TAA — single view only

## Decisions

### D1: Jitter compensation via UV offset in TAA shader

**Decision**: Pass `u_taa_jitter_uv` (vec2 in UV units) to `taa.glsl`. The shader samples `u_current_color` at `v_uv + u_taa_jitter_uv` to reconstruct the pixel-center color. The history reprojection remains `v_uv - velocity.xy` (history is the temporally-accumulated un-jittered output, so it needs no jitter compensation).

**Confirmed correct (2026-07-23)**: The addition sign is intentional. NDC Y-axis is reversed relative to UV Y-axis in this engine's FBO setup. Both X and Y jitter compensation use addition:
- X: `proj[2][0] += 2*jx/W` → image shifts LEFT → `R(p) = S(p + jx/W)` → compensate with `+ jx/W`
- Y: `proj[2][1] += -2*jy/H` → image shifts UP in NDC = DOWN in UV → `R(p) = S(p + jy/H)` → compensate with `+ jy/H`

**Velocity note**: Velocity is computed from un-jittered projview matrices. The TAA history buffer stores the temporally accumulated result which is approximately un-jittered (jitter averages out over frames), so `prev_uv = v_uv - velocity` is correct without additional jitter compensation.

**Alternatives considered**:
- Bake jitter into velocity and have TAA undo it: More complex, requires changing velocity semantics.
- Use bicubic/gather4 for current color: Higher quality but more expensive; can be added as a follow-up.

### D2: Adaptive blend with variance-based fallback

**Decision**: Replace the global `taa_blend` with a per-pixel blend computed as:
```
local_variance = compute_variance(current_color, 3x3_neighborhood)
blend = base_blend / (1.0 + variance_scale * local_variance)
blend *= edge_stop_factor(reprojected_history, neighborhood)
```

Keep `taa_blend` as a tunable `base_blend` in settings (default 0.1–0.2, since per-pixel adaptation handles the heavy lifting). This inverts the semantics: the setting now controls the **minimum** history contribution rather than the fixed contribution.

**Rationale**: Prevents ghosting in high-frequency areas while allowing strong accumulation in flat regions. The per-pixel approach is standard across all modern TAA implementations.

### D3: Variance-guided clip with 5×5 neighborhood

**Decision**: Expand the neighborhood to 5×5 for AABB computation, and expand the clip box by `γ · σ` (γ = 1.0 default, tunable).

**Rationale**: 5×5 provides enough samples for stable variance estimation. The variance expansion prevents over-clipping of valid high-frequency detail. This is the single most impactful quality improvement after jitter compensation.

**Alternative**: Use a depth-aware clip (reproject depth, compare depths). More complex; can be added later if needed for thin-geometry disocclusion.

### D4: Contrast-adaptive sharpen as separate pass

**Decision**: Add a new `taa_sharpen.glsl` shader and `add_taa_sharpen_pass()` to `RenderShared`. The sharpen uses a luma-guided unsharp mask:

```
sharpened = color + sharpen_strength * (color - blur3x3(color))
sharpened = mix(color, sharpened, edge_mask)  // don't sharpen flat areas
```

The sharpen runs on the TAA output before bloom.

**Rationale**: Separating sharpen from the TAA resolve keeps the resolve shader simple. The sharpen pass is trivially disabled/tuned independently. A full CAS implementation can replace the simple unsharp mask in a follow-up.

### D5: Keep YCoCg for color-space operations

**Decision**: Continue using YCoCg for neighborhood analysis and clipping. YCoCg separates luminance from chrominance, which is beneficial for variance estimation and clipping.

**Rationale**: YCoCg is well-established for TAA (Playdead, INSIDE). It provides better results than RGB-space clipping at negligible cost. Changing color spaces would be a regression.

## Risks / Trade-offs

- **[Risk] Performance regression** — 5×5 neighborhood sampling (25 taps) + sharpen pass add GPU cost.
  → **Mitigation**: The TAA pass already samples 3×3 (9 taps); 5×5 is 25 taps. Use textureGather or compute a running min/max to reduce taps. Profile before/after. Target: <2ms total at 1080p.

- **[Risk] Over-sharpening artifacts** — A fixed sharpen strength can cause haloing on high-contrast edges.
  → **Mitigation**: Luma-guided edge-stop in the sharpen pass prevents sharpening across edges. Expose sharpen strength as a tunable setting.

- **[Risk] Blend factor semantics change** — `taa_blend` currently means "history weight" (0.9 = 90% history). After the change, it becomes "base new-sample weight" (0.1 = 10% current). This inverts the meaning and may confuse existing users.
  → **Mitigation**: Rename to `taa_feedback` or keep name with updated documentation. The Python binding will reflect the new semantics.

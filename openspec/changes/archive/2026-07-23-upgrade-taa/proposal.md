## Why

The current TAA implementation produces visible frame-to-frame shakiness even when the camera is stationary. The algorithm is a basic TAA (3×3 YCoCg neighborhood clamp + fixed 0.9 blend factor) that lacks several key techniques found in modern TAA implementations (UE4/5, Unity HDRP, Playdead's TAA). The root causes are:

1. **Jitter offset not communicated to the TAA resolve shader** — the Halton jitter applied to the projection matrix is never passed to `taa.glsl`, so the resolve cannot compensate for sub-pixel sample offsets. Each frame's accumulated result includes a different sub-pixel bias, causing persistent shimmer.
2. **No adaptive blend factor** — the fixed `taa_blend = 0.9` does not respond to disocclusion, motion magnitude, or local color variance, causing ghosting that manifests as shake in high-frequency regions.
3. **Overly restrictive neighborhood clamp** — the 3×3 AABB in YCoCg clips history too aggressively, preventing proper temporal accumulation and causing flicker on thin geometry.
4. **No sharpening pass** — the softened TAA output lacks a contrast-adaptive sharpen to restore lost detail, exacerbating the visual impression of instability.

## What Changes

Upgrade the TAA pipeline to match current mainstream quality:

- **Jitter compensation**: Pass the per-frame jitter offset (in UV space) to `taa.glsl` so the resolve can sample `u_current_color` at the pixel center, aligning current and history sample positions.
- **Adaptive blend factor**: Replace the fixed `u_taa_blend` with a per-pixel blend computed from local neighborhood color variance — areas with high variance (edges, disocclusion) blend less history.
- **Variance-guided clip**: Expand the AABB clip based on neighborhood variance (matching UE4's technique) to avoid over-clamping in noisy regions while still rejecting invalid history.
- **TAA sharpen pass**: Add a lightweight contrast-adaptive sharpening pass (e.g., AMD FidelityFX CAS or a simple unsharp mask) applied after TAA resolve to restore high-frequency detail.
- **Velocity refinement**: Ensure velocity correctly encodes both camera motion and the jitter offset so reprojection is accurate.

## Capabilities

### New Capabilities

- `taa-jitter-compensation`: TAA shader receives per-frame jitter UV offset and uses it to reconstruct pixel-center color before blending with history.
- `taa-adaptive-blend`: Per-pixel blend factor derived from local color variance; replaces the global `taa_blend` uniform.
- `taa-variance-clip`: AABB clip expanded by neighborhood variance to reduce over-clamping in noisy regions.
- `taa-sharpen`: Post-TAA contrast-adaptive sharpen pass to restore lost high-frequency detail.

### Modified Capabilities

- `taa-settings`: GlobalSettings may gain new tunables (sharpen strength, variance clip scale); the existing `taa_blend` may be repurposed or deprecated.

## Impact

- `engine/content/shader/taa.glsl` — full rewrite of the resolve shader
- `engine/content/shader/include/uniforms.glsl` — add `u_taa_jitter_uv` uniform
- `engine/runtime/source/render/global.h` — add jitter-offset fields, sharpen settings
- `engine/runtime/source/render/global.cpp` — propagate new settings
- `engine/runtime/source/render/renderer/render_shared.h/.cpp` — pass jitter offset to TAA pass, add sharpen pass
- `engine/runtime/source/render/renderer/renderer_deferred.cpp` — update TAA integration
- `engine/runtime/source/render/renderer/renderer_forward.cpp` — update TAA integration
- `engine/content/shader/` — new `taa_sharpen.glsl` shader file
- `engine/runtime/source/python/py_engine.gen.cpp` — regenerate bindings for new settings

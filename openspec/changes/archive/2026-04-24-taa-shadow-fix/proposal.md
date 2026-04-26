# Fix TAA jitter (skybox shaking) and Shadow-pass empty commit

## Summary

Two rendering bugs have been observed in the current branch:

1. Temporal Anti-Aliasing (TAA) produces excessive per-frame jitter — the entire scene, including the skybox, appears to shake when the camera is stationary.
2. The shadow map pass reports committing 0 objects (shadowpass logs show 0 committed objects), resulting in no shadows being drawn.

This document describes how to reproduce, investigate, and validate fixes.

## Reproduction steps

1. Launch the editor/game with a scene that includes a skybox and at least one shadow-casting mesh and a directional or point light.
2. Enable TAA in graphics settings and enable shadow mapping.
3. Observe the scene while keeping the camera stationary: the skybox and scene geometry should remain steady; current behavior: visible frame-to-frame displacement (shaking).
4. Log shadow pass statistics (the engine exposes a shadowpass commit count). Confirm the commit count is 0.
5. Toggle TAA off — the shaking should stop if TAA is the source. Confirm shadowpass remains at 0 objects.

## Investigation checklist

TAA (jitter / skybox shaking):

- [ ] Locate the code that generates per-frame jitter offsets (common files: engine/render/taa.* or engine/render/projection.*). Inspect sequence pattern (Halton, Hammersley, or random) and amplitude.
- [ ] Verify how jitter units are defined (pixels vs. NDC). Confirm conversion to clip/NDC space uses the render-target resolution (not window resolution or an inverted dimension).
  - Guidance: normalize pixel jitter by resolution: jitter_ndc_x = (2.0 * jitter_px_x) / render_width; jitter_ndc_y = (2.0 * jitter_px_y) / render_height. Confirm engine projection convention before applying.
- [ ] Confirm where the jitter is applied to the projection matrix. Detect whether the matrix element indices used match the engine's row/column convention (row-major vs column-major).
- [ ] Verify storage of previous-frame matrices used by TAA reprojection/motion vectors. Ensure previousView, previousProj (and combined previousViewProj) correspond to the matrices that were used to render the previous frame (inclusive or exclusive of jitter depending on motion-vector convention).
- [ ] Check motion-vector generation: ensure velocities are calculated using the same matrix conventions and include/exclude jitter consistently with TAA reprojection.
- [ ] Inspect skybox render path: determine whether skybox rendering uses the same (jittered) projection. If skybox is drawn with jitter, it will appear to move. Verify and, if necessary, render skybox with an unjittered projection (camera rotation only).
- [ ] Check depth-prepass/resolve interactions: confirm depth/stencil content is stable across jittered frames or accounted for in reprojection.
- [ ] Add per-frame debug output: current jitter (pixels), jitter_ncd/clip, previous/current view/proj matrices, and a small overlay showing offset magnitude in pixels.

Shadow-pass (0 committed objects):

- [ ] Identify the shadow pass code path and the routine that builds the shadow draw list (candidate: engine/render/shadowpass.*).
- [ ] Verify which matrices (camera vs. light) are used for frustum building and culling. Shadow culling must use light-space matrices; if the camera or a jittered projection is used accidentally, frustum may be degenerate.
- [ ] Dump the frustum planes and list candidate objects considered for shadow casting for a frame. For each object, print the culling decision (in/out) and the reason (outside frustum, flagged as non-shadow-caster, filtered by mask, LOD rejected, etc.).
- [ ] Confirm visibility / layer masks: ensure shadow-caster flags or visibility masks have not been changed and are still set on meshes.
- [ ] Inspect the commit/append logic in the shadowpass; verify there is no early return or inverted condition that prevents adds into the command buffer when objects pass culling.
- [ ] Add temporary logging in the commit path to emit the number of candidates, number accepted, and per-object rejection reasons.

## Hypotheses (likely root causes)

TAA:
- Jitter units or normalization are incorrect (e.g. using window size instead of render-target size, or reversed sign), resulting in exaggerated offsets.
- Jitter or reprojection matrices are mis-applied to the skybox; skybox should be rendered with an unjittered projection or excluded from TAA reprojection.
- Previous-frame matrices saved for reprojection are incorrect (e.g., saved after applying current jitter instead of the actual previous jitter), causing the reprojection to compute large deltas.
- Motion vector generation uses inconsistent matrix conventions or omits jitter, causing incorrect reprojection weights.

Shadow:
- Shadow culling uses wrong matrices (camera jitter mistakenly applied, or light matrices are degenerate), so all objects are culled.
- Shadow-caster filter or visibility masks were accidentally changed and exclude all meshes.
- The commit path contains a logical bug (early-out, inverted condition) preventing objects from being appended.

## Proposed fixes (high-level)

TAA:
- Standardize jitter computation and conversion:
  - Generate jitter in pixel space using Halton or TAA sequence.
  - Convert to NDC: jitter_ndc = (2.0 * jitter_px) / render_target_size.
  - Apply jitter to the projection matrix entries that shift clip-space X/Y, respecting engine matrix layout.
- Maintain both jittered and unjittered projection matrices per frame:
  - Store previous_jittered_proj and previous_unjittered_proj as needed.
  - Save previous matrices before applying the current frame's jitter.
- Motion vectors:
  - Ensure motion-vector shader uses the same matrix conventions and previous matrices that include the jitter used when the previous frame was rendered.
- Skybox:
  - Render skybox using the unjittered projection (camera rotation only) so it remains stable.
  - Alternatively, render skybox before TAA or exempt it from reprojection.

Shadow:
- Ensure shadow culling uses correct light-space matrices (do NOT use a jittered camera projection)
- Add instrumentation in culling/commit logic to log per-object decisions and fix any inverted conditions that prevent commits.
- Add a minimal automated test (single cube + light) to assert shadowpass commit count > 0.

## Debugging & instrumentation

- Add a temporary debug overlay that prints per-frame jitter (in px), previous/current view/proj matrices (hash or truncated floats), and shadowpass candidate statistics.
- Add a selective log category (e.g., `render.taa` and `render.shadow`) controllable via runtime console to avoid log spam.
- Capture a frame trace (if engine supports GPU profiling/frame capture) when the issue reproduces and inspect reprojection and culling matrices in the trace.

## Candidate files to inspect

- engine/render/taa.cpp, taa.h
- engine/render/projection.cpp
- engine/render/motion_vectors.cpp
- engine/render/skybox.cpp
- engine/render/shadowpass.cpp
- engine/render/culling.cpp / .h

## Acceptance

Follow the acceptance criteria in the YAML manifest. Provide screenshots (before/after) or a short recording demonstrating the skybox stability and presence of shadows in the test scene.

Additionally, verify the build and smoke test using dev\z1.py: run "python dev\\z1.py smoke" from the repository root. The command must complete with exit code 0 and perform the repro scene verification (shadowpass commit_count > 0). Include CI logs or console output as evidence.

# Tasks: TAA jitter & Shadow-pass fix

This task list breaks the work into small, reviewable steps. Implementers should prefer small PRs addressing one small area at a time (diagnostics → fix → tests).

## 1. Diagnostic / Trace (diagnostic/trace)
- Goal: Gather precise runtime data to verify hypotheses.
- Actions:
  - Add shadowpass logging: number of candidates, number accepted.
  - `smoke` echoes engine stdout so counts are visible in CI.
- Status: [x] DONE — `add_shadow_pass` and `add_velocity_pass` now call `stats.begin_counter` / `increment_counter` / `end_counter` per CSM cascade. `std::cout` prints `candidates` and `drawn` every 60 frames (frame 0 always fires during smoke). `smoke.py` echoes all engine stdout lines.

## 2. Create minimal repro scene (repro/scene)
- Status: [x] SKIPPED — default scene (new_scene.yaml) was sufficient to reproduce both issues and verify fixes via `python dev\z1.py smoke`.

## 3. Fix: TAA skybox jitter (fix/taa-jitter)
- Goal: Skybox must not shake when camera is stationary.
- Status: [x] DONE — `add_gbuffer_pass` now accepts `unjittered_projview`; skybox uses `glm::inverse(unjittered_projview)` instead of the jittered `g->projview`.
  - Files changed: `renderer_deferred.h`, `renderer_deferred.cpp`

## 4. Fix: Motion vectors for no-material objects (fix/motion-vectors)
- Goal: Objects that rely on the default material must emit correct velocity vectors.
- Status: [x] DONE — `add_velocity_pass` was skipping all primitives with empty material GUIDs (`if (!mi) continue`). Added `if (!mi) mi = default_material` fallback (same pattern as GBuffer pass). TAA jitter artifacts on no-material objects eliminated.
  - Files changed: `render_shared.h`, `render_shared.cpp`, `renderer_deferred.cpp`, `renderer_forward.cpp`

## 5. Fix: Shadow-pass 0 draws (fix/shadow-commit)
- Goal: shadowpass commit_count > 0 for any scene with meshes.
- Status: [x] DONE — root cause was `add_shadow_pass` skipping all primitives with empty material GUIDs. Added `if (!mi) mi = default_material` fallback. Verified via smoke: all 4 CSM cascades show `candidates=3 drawn=3`.
  - Files changed: `render_shared.h`, `render_shared.cpp`, `renderer_deferred.cpp`, `renderer_forward.cpp`

## 6. Tests & Automation (test/automation)
- Status: [x] DONE — `python dev\z1.py smoke` exits 0 and prints per-cascade shadow counts, confirming `drawn > 0`.

## 7. QA, Review & Documentation (docs/review)
- Status: [x] DONE — openspec tasks updated, change archived, changes committed.


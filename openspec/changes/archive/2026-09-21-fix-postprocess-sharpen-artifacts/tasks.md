## 1. Remove the legacy post-process sharpen

- [x] 1.1 Delete the `sharpen()` helper from `engine/content/shader/postprocessing.glsl`
- [x] 1.2 Replace `vec3 color = sharpen(u_scene, v_uv);` with a single `v_uv` scene-color fetch in `main()`
- [x] 1.3 Confirm no other pass/shader references the removed helper (grep for `sharpen` in
      `engine/content/shader/`)
- [x] 1.4 Add `color = max(color, vec3(0.0));` between the bloom composite and the exposure multiply in
      `engine/content/shader/postprocessing.glsl`

## 2. Validation gates

- [x] 2.1 `python dev/z1.py compile` passes with 0 errors (0 warnings, 1.4s)
- [x] 2.2 `python dev/z1.py validate-shaders` passes (pre-existing macOS `sprite_2d_batched.glsl` failure
      excepted; `postprocessing.glsl` validated fully)
- [x] 2.3 `python dev/z1.py format --dry-run` reports no new drift in the edited shader

## 3. Runtime verification (editor captures)

- [x] 3.1 Capture with `Z1_MSAA=1`, `Z1_MSAA=2`, `Z1_MSAA=4` (`--frames=90 --screenshot=true`, display
      awake): the 5-pixel cross detector reports zero white and zero black plus clusters in all three
      (pre-fix: 4 / 7 / 9 plus clusters; white components 93 / 99 / 98 → 12 / 12 / 16)
- [x] 3.2 Verify the removed filter does not regress the rest of the image: bloom/exposure/tonemap output
      matches the reference capture with only the sharpening delta removed (26.9% of pixels touched, 91.7%
      of the larger deltas match the sharpen kernel sign; the largest deltas are the former negative-wrap
      artifacts in the dark dot pattern, now correct)
- [x] 3.3 `python dev/z1.py smoke --frames 10` passes with 0 GL errors (2.3s)
- [x] 3.4 Capture after the negative clamp is added and confirm the output is byte-identical to the captures
      taken before the clamp (the clamp must be a no-op for valid non-negative radiance) — byte-identical
      (md5) at MSAA Off/2x/4x
- [x] 3.5 Confirm the fixed capture still renders correctly (visual check of the helmet scene: sky, IBL
      reflections, bloom and shadows unchanged)

## 4. Close-out

- [x] 4.1 `python dev/z1.py dcv --auto` run to completion (macOS: stops at the pre-existing
      `sprite_2d_batched.glsl` shader-validation failure — Apple's 16-sampler limit; compile and format
      steps pass; the benchmark step is Windows-shaped) 
- [x] 4.2 Update `openspec/kb/render-pipeline.md` post-process table/notes (sharpen is exclusively the TAA
      sharpen pass)
- [x] 4.3 Archive the change (`openspec archive fix-postprocess-sharpen-artifacts --yes --json`) — archived as `2026-09-21-fix-postprocess-sharpen-artifacts`

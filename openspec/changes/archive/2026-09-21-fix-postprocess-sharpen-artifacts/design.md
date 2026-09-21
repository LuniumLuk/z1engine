## Context

`postprocessing.glsl` is the last pass of both the deferred and forward pipelines. Its `main()` starts with
`vec3 color = sharpen(u_scene, v_uv);` — a hardcoded unsharp mask (`amount = 0.5`) whose blur term is the
average of the 4 axial neighbours (up/down/left/right). The kernel is therefore a 5-tap cross:
`out = 1.5*c - 0.125*(N+S+E+W)`. It runs unconditionally: no setting, no toggle, in every frame since before
the macOS port (introduced with the original post-process pass).

The engine already has a dedicated, settings-gated sharpen: `taa_sharpen.glsl` (3×3 unsharp with an
edge-stop, behind `taa_enabled && taa_sharpen_enabled`, strength `taa_sharpen_strength`), documented in the
KB and presets spec. The post-process sharpen is a legacy duplicate that also fires when the user has
sharpening disabled.

Investigation evidence for this change (editor capture via `--screenshot=true`, 640x480, MSAA Off/2x/4x):

- Detector: isolated extreme pixels (white: min channel ≥ 250; black: max channel ≤ 5) were analysed with
  connected components; the artifact signature is exactly a 5-pixel plus (center + 4 axial neighbours),
  e.g. at (341,114) in the helmet view. All three pre-fix captures are byte-identical (deterministic).
- A/B capture with the post-process `sharpen()` call bypassed: white components 93 → 0 and black
  components 25 → 0, all plus clusters gone (0/0 at MSAA Off/2x/4x, 2 identical captures per setting).
- Mechanism: the kernel is `out = 1.5*c - 0.125*(N+S+E+W)`, so a single bright pixel (the IBL specular
  sparkle, confirmed by isolating the deferred lighting shader) drives the 4 axial neighbours strongly
  negative. `tonemap_reinhard(color) = color / (color + 1)` has a **pole at -1**: inputs below -1 come out
  > 1 (saturated white) and inputs just below 0 make the gamma `pow()` NaN (black). The 5-pixel cross is
  the kernel's support footprint, not a filter footprint.
- Confirmation: re-adding the sharpen with `max(result, 0.0)` (negative clamp) removes all white crosses but
  replaces them with ~400 tiny black components — the clamped undershoot. This isolates the artifact chain
  as "kernel negative lobes → non-monotonic tone curve (unclamped) or clamped-to-black (clamped)".
- With MSAA on the artifact count grows (4 → 7 at 2x, 9 at 4x): MSAA-resolved edge pixels are sharper
  single-pixel signals, so the same kernel drives larger negative lobes. MSAA itself is not the cause — the
  artifact reproduces identically with `msaa_samples = Off`.

## Goals / Non-Goals

**Goals:**

- Eliminate the 5-pixel cross and white/black salt-and-pepper artifacts from the final image in both
  renderers and at every MSAA level.
- Make the post-process pass a pure color pipeline (bloom composite → exposure → tonemap → tint → gamma).
- Keep sharpening available exactly as documented: the gated, edge-aware TAA sharpen pass.

**Non-Goals:**

- Changing MSAA resources, resolve, or edge-shading code (not the cause).
- Reworking `taa_sharpen.glsl` or its settings.
- Adding a "sharpen without TAA" feature (the dedicated pass intentionally requires TAA).

## Decisions

### D1: Remove the hardcoded sharpen from `postprocessing.glsl` (do not gate it)

Delete the `sharpen()` helper and call `texture(u_scene, v_uv)` directly in `main()`.

Alternatives considered:

- **Gate it behind `u_taa_sharpen_enabled`**: rejected — with TAA on and sharpening enabled the image would
  be sharpened twice (post-process cross + TAA 3×3 pass), and the cross kernel is strictly worse than the
  edge-aware one (it cannot suppress halos and imprints the 5-pixel cross).
- **Replace its kernel with the TAA-style 3×3 unsharp and gate it**: rejected — duplicates the dedicated
  pass, and would still apply sharpening in pipelines where the user disabled it.
- **Clamp the sharpen output (firefly clamp)**: rejected — a band-aid; the cross-footprint overshoot and
  the noise amplification remain.

Root-cause evidence (captures with `sharpen()` bypassed: 0 extreme components; with it: 93 white/25 black at
MSAA off, 9 plus clusters at 4x) makes removal the minimal, verifiable fix. Keeping the kernel (even
clamped) is rejected because it turns the undershoot into black speckle (measured: ~400 black components)
and it is an always-on filter no setting or preset can disable.

### D2: Clamp the assembled color to non-negative values before exposure and tone mapping

Add `color = max(color, vec3(0.0));` after the bloom composite in `postprocessing.glsl`.

Rationale: the Reinhard curve is not monotonic below -1 (pole at -1) and `pow()` is undefined for negative
bases, so any negative radiance — from a filter, a future pass, or content — becomes saturated white or
NaN-black. Radiance is non-negative by construction, so clamping is lossless for correct inputs and turns
the whole class of failure into "no effect" for the reported artifact chain. The TAA sharpen pass already
clamps its own output (`result = max(vec3(0.0), result)`); this guard generalizes that protection to the
final composite.

Alternatives considered:

- **Clamp only inside `tonemap_reinhard`**: rejected — tint/gamma after tone mapping would still see
  negatives (NaN in `pow()`).
- **No clamp (just remove the sharpen)**: rejected as insufficient hardening — the tone curve remains a
  trap for any future negative input and the fix would silently regress if a spatial filter is re-added.

### D3: Keep sharpening solely in the TAA sharpen pass

No behavior change to `taa_sharpen.glsl`, its uniforms, or the pass wiring. The presets spec already
documents the sharpen as a TAA companion (`taa_enabled` / `taa_sharpen_enabled` table), so no requirement
update is needed there.

### D4: New capability spec `postprocess-color-pipeline`

The pass composition becomes an explicit, testable requirement (non-spatial operations only) so the legacy
filter cannot silently return via a future pass.

## Risks / Trade-offs

- [Default images become slightly softer — the hardcoded 0.5 sharpen was always on] → Accepted: the filter
  produced visible artifacts; the documented sharpening path (`taa_sharpen_enabled`) restores crispness
  when desired.
- [Users who relied on the post-process sharpen without TAA lose sharpening] → Accepted and documented as
  a non-goal; enabling TAA + sharpen is the supported path.
- [Scenes/art tuned around the sharpened look may shift slightly] → Accepted: the change removes a defect;
  no settings or scene data change.

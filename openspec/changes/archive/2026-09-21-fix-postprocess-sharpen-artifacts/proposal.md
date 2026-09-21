# Fix post-process cross-kernel sharpen artifacts

## Why

The final render contains many saturated white or black pixels arranged as 5-pixel crosses on smooth
surfaces (worst with MSAA enabled, but present with MSAA off). Bisection localized the cause to the
post-process pass (`postprocessing.glsl`), which unconditionally applies a hardcoded 5-tap cross-kernel
unsharp mask (`amount = 0.5`, no setting). The kernel's negative lobes (it subtracts the 4 axial
neighbours) produce **negative HDR values**, and the Reinhard tone curve `color / (color + 1)` has a pole
at -1: negative inputs wrap to values > 1 (saturated white) while values just below 0 make `pow()` produce
NaN (black). Experiments confirm both halves of the chain: bypassing `sharpen()` removes every artifact,
and re-adding it with a `max(..., 0.0)` clamp replaces the white crosses with black speckle (the clamped
undershoot). Sharpening is supposed to be the settings-gated TAA sharpen pass; the post-process copy
duplicates it, fires even when `taa_sharpen_enabled` is false, and feeds unclamped negatives into a
tone curve that is not monotonic below -1.

## What Changes

- Remove the hardcoded `sharpen()` function and its unconditional call from `postprocessing.glsl`.
- The post-process pass becomes a pure color pipeline: bloom composite → negative clamp → exposure →
  Reinhard tonemap → tint → gamma, with no spatial (neighbourhood) filtering.
- Clamp the assembled HDR color to non-negative values before exposure/tone mapping so a non-monotonic
  tone curve can never wrap negative inputs into bright output (or NaN into black).
- Spatial sharpening remains exclusively the dedicated `taa_sharpen.glsl` pass, gated by
  `taa_enabled && taa_sharpen_enabled` with `taa_sharpen_strength` (unchanged).
- No settings, APIs or C++ code change; both deferred and forward renderers are fixed by the shared shader.

## Capabilities

### New Capabilities

- `postprocess-color-pipeline`: The final post-process pass applies only non-spatial color operations
  (bloom composite, exposure, tonemap, tint, gamma); sharpening is exclusively the gated TAA sharpen pass.

### Modified Capabilities

- None.

## Impact

- `engine/content/shader/postprocessing.glsl` (shader only, hot-reloadable; no rebuild required for the fix
  to take effect, but the repository build must still be validated).
- Visual: default images lose the legacy hardcoded sharpening; the cross/salt-and-pepper artifacts and the
  MSAA-related amplification disappear. Users who want sharpening enable the existing TAA sharpen setting.
- No impact on APIs, dependencies, presets, MSAA resources or pass graphs.

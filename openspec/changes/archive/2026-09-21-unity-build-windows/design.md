# Design: unity builds on Windows

## Context

The macOS change generates its own blob translation units with `unity_blob_project()` (root `premake5.lua`) because
premake 5.0.0-beta8 implements `enableunitybuild` only for VS actions. Windows therefore has *two* possible routes.

## Decision 1: reuse the generated blobs instead of premake's `enableunitybuild`

Chosen: extend the existing blob generator to `system:windows`.

- One mechanism, one layout: `engine/intermediate/unity/<project>/<project>_unity_<n>.cpp`, same sorted round-robin
  grouping (8 sources per blob), same exclusions, same determinism — so a build difference between platforms is a
  compiler difference, not a generator difference.
- The exclusions (vendor `*_build.cpp`, `pch.cpp`, `source/3rdparty/*.cpp`) are already encoded once and are the part
  most likely to need maintenance.
- Rejected: `unitybuild "On"` + per-file `unitybuild "Off"`. Fewer lines, but MSBuild then decides the batching (no
  control over blob size), the unity files are invisible to `generate` (harder to review/regenerate deterministically),
  and the two platforms would diverge in exactly the cases that hurt.

Trade-off accepted: blobs are regenerated only on `generate --unity`, so a newly added source requires a regenerate
(same as macOS, and `generate` is already mandatory after adding files).

## Decision 2: MSVC needs the PCH in the blob itself

`error C1010: unexpected end of file while looking for precompiled header` for every runtime blob. MSVC's `/Yu`
mechanism requires the precompiled header to appear **textually first in the compiled translation unit and spelled
exactly like the `/Yu` argument**; the first line of a blob including a `.cpp` whose first line includes `pch.h` is not
enough, and a path-qualified spelling (`"../../../runtime/source/pch.h"`) is not accepted either — both were tried and
both still produced C1010.

Solution: `unity_blob_project(project_dir, project_name, pch_name)` writes `#include "<pch_name>"` as the first line
when a PCH is given; `runtime` passes `"pch.h"`, the other three projects have no PCH and pass nothing. Clang/macOS is
unaffected (it tolerates the extra first include).

## Decision 3: `/bigobj` for unity builds on Windows

Debug fails with `error C1128: number of sections exceeded object file format limit` on the `editor` blob (its ~30
sources at `/Od` with `/RTC1` emit more sections than the COFF limit). `/bigobj` is applied at workspace level under
`system:windows` + `options:unity`, so all engine blobs get it and per-TU builds stay untouched (the runtime project's
existing per-TU `/bigobj` is unrelated and kept).

## Decision 4: fix the Profile PhysX library path (pre-existing)

Benchmarking all four configurations exposed that `Profile` never linked: `engine/runtime/premake5.lua` gave PhysX
`libdirs` only for `Debug` and `Release or Hybrid`. PhysX ships `Debug` and `Release` only, so Profile now shares the
Release binaries, matching the config's "optimized + symbols + asserts" intent.

## Measurement protocol

- Clean build: `Remove-Item -Recurse engine/intermediate` before each run, then `generate [--unity]`, then
  `compile --config <Cfg>`; timing from the dev CLI `RESULT` elapsed field (wall clock incl. MSBuild overhead).
- All 23 projects (engine, tools, 11 test executables), same project parallelism in both modes, two runs per cell.
- Incremental: timestamp `engine/runtime/source/scene/animation_system.cpp`, then `compile` on an up-to-date tree.

## Results (Windows, AMD Ryzen 7 9800X3D 8C/16T, 47 GB, VS2026)

| Configuration | per-TU (run A/B) | `--unity` (run A/B) | Speed-up | `.obj` total per-TU → unity |
|---------------|------------------|---------------------|----------|-----------------------------|
| Debug | 84.7 / 84.6 s | 53.8 / 53.2 s | 1.58x | 638 MB → 432 MB |
| Release | 138.9 / 136.7 s | 86.0 / 86.0 s | 1.60x | 259 MB → 179 MB |
| Profile | 126.0 / 127.4 s | 75.1 / 75.1 s | 1.69x | 317 MB → 225 MB |
| Hybrid | 142.0 / 142.9 s | 86.6 / 87.9 s | 1.63x | 270 MB → 180 MB |

Incremental (Hybrid, touch one runtime source): per-TU **6.9 s** (no-op 1.6 s) vs unity **10.2 s** (the blob holding
that source is recompiled, then the dependents relink). Unity therefore stays a cold/CI mode; the default remains the
interactive one — same conclusion as macOS, but with a much smaller incremental penalty on Windows.

Composition (unchanged from macOS): runtime = 8 blobs covering 57 sources + 8 standalone TUs (6 vendor `_build.cpp`,
`pch.cpp`, `source/3rdparty/imgui_layer.cpp`); bakery/editor/game = 1 blob each. Windows sees the same split because
premake reports matched paths with forward slashes, so the `/3rdparty/` exclusion works as written.

## Risks

- Blob grouping changes when sources are added/removed → regenerate (`generate --unity`); stale blobs are harmless
  because they are in the gitignored `engine/intermediate/unity/` tree.
- Unity and per-TU objects are interchanged without MSBuild noticing a flag change; toggling `--unity` should be
  followed by a clean (`engine/intermediate` removal) for trustworthy timings. `generate` already wipes
  `engine/intermediate/Hybrid` for the probing toggle and could be extended to the unity toggle later.
- Unity blobs are more memory hungry than a single TU; the dev CLI's `--jobs` option only applies to macOS builds, so
  Windows memory pressure has to be handled by closing other workloads.

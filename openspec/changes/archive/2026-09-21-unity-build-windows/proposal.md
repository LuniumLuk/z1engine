# Proposal: extend the opt-in unity build to Windows

## Why

The unity build (2026-09-21) is scoped to macOS: the four engine projects only call `unity_blob_project()` under
`{ "system:macosx", "options:unity" }`. Windows pays the same avoidable cost the macOS port paid — a cold Hybrid build
takes 142 s on the Windows dev machine, and MSBuild compiles the files of a project **serially** (no
`MultiProcessorCompilation`; `devenv.com /Build` only parallelizes across projects), so the runtime project's 57
translation units dominate the critical path. Merging them into 8 blobs cuts that path without touching flags.

Measured on the Windows reference machine (AMD Ryzen 7 9800X3D 8C/16T, 47 GB, VS2026 `devenv.com /Build`, clean =
`engine/intermediate` removed, all 23 projects, two runs each):

| Configuration | per-TU | `--unity` | Speed-up |
|---------------|--------|-----------|----------|
| Debug | 84.7 / 84.6 s | 53.8 / 53.2 s | 1.58x |
| Release | 138.9 / 136.7 s | 86.0 / 86.0 s | 1.60x |
| Profile | 126.0 / 127.4 s | 75.1 / 75.1 s | 1.69x |
| Hybrid | 142.0 / 142.9 s | 86.6 / 87.9 s | 1.63x |

## What Changes

- `python dev/z1.py generate --unity` now also applies on Windows: runtime/editor/game/bakery build from the same
  generated blobs (`engine/intermediate/unity/<project>/`) the macOS path already uses, via
  `filter { "system:macosx or system:windows", "options:unity" }`.
- **MSVC support in the blob writer**: blobs of a project that uses a precompiled header start with
  `#include "pch.h"` — MSVC's `/Yu` check (`C1010`) requires the PCH textually first and spelled exactly like the
  `/Yu` argument, and including another source file (or a path-qualified spelling) does not satisfy it.
- **Windows unity builds compile with `/bigobj`** (workspace-level, unity-only filter): the Debug `editor` blob
  exceeds the default object-file section limit (`C1128`).
- **Pre-existing Profile-config bug fixed**: the PhysX `libdirs` filter listed only `Debug` and `Release or Hybrid`,
  so `Profile` had no PhysX path — `runtime` failed (`LNK1181: PhysX_static_64.lib`) and 13 dependent projects failed
  with `LNK1104: runtime.lib`. Profile now shares the Release PhysX binaries, like Release/Hybrid.
- Docs/spec: `openspec/kb/build.md` records the Windows protocol and numbers; the `unity-build` spec becomes
  platform-neutral.

## Capabilities

### Modified Capabilities

- `unity-build`: the opt-in unity mode now covers Windows as well as macOS — platform-neutral blob composition,
  MSVC-specific constraints (PCH include, `/bigobj`), measured improvement floors per platform, and an unchanged
  default path.

## Impact

- **Build system**: root `premake5.lua` (shared blob writer with an optional PCH include; workspace-level `/bigobj`
  under `options:unity` on Windows), `engine/{runtime,editor,game,bakery}/premake5.lua` (platform filter; runtime
  passes its PCH name).
- **Dev CLI**: `dev/commands/generate.py` (`--unity` help/info text no longer say "macOS").
- **Behaviour**: none at runtime. Per-TU builds are unchanged except the fixed Profile PhysX library path; shaders,
  assets and engine code are untouched.
- **Verification**: unity builds 23/23 projects in all four configurations with 0 errors / 0 warnings; per-config test
  results are identical to the per-TU control (Debug 13/13, Release 9/10, Profile 12/13, Hybrid 11/11 — the
  `test_render_graph` crash in Release/Profile is pre-existing and appears in both modes); 120-frame smoke run clean;
  `--unity --probing` also builds.

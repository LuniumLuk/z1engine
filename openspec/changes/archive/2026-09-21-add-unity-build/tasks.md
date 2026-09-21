# Tasks: opt-in unity builds for macOS

## 1. Source-level prerequisites

- [x] 1.1 `engine/runtime/premake5.lua`: fix the define spelling `glfw_INCLUDE_NONE` → `GLFW_INCLUDE_NONE` (both blocks) — the typo made every TU including `glfw3.h` parse Apple's legacy `<OpenGL/gl.h>`; fixing it alone improved the per-TU runtime build 167 s → 147 s (harness)
- [x] 1.2 Add `#pragma once` to the headers that lack it (5 files: `bakery.h`, `picking_system.h`, `renderer_2d.h`, `base.inl`, `string_utils.h`)
- [x] 1.3 Regenerate + rebuild (default mode): 241.8 s, 0 errors / 0 warnings

## 2. Premake unity support

- [x] 2.1 Root `premake5.lua`: `--unity` option + `unity_blob_project()` helper (collects `source/**.cpp`, skips `3rdparty/*` / `*_build.cpp` / `pch.cpp`, sorted round-robin into `max(1, ceil(n/8))` groups, writes blobs to `engine/intermediate/unity/<project>/`, then `removefiles` originals + `files` blobs). Include paths are computed explicitly because premake's `path.getrelative` resolved against the calling script here
- [x] 2.2 Engine project files (runtime/editor/game/bakery) call the helper under `{ "system:macosx", "options:unity" }`
- [x] 2.3 `dev/commands/generate.py`: `--unity` passthrough + info line
- [x] 2.4 Verified: 8 runtime blobs (8/7/7/7/7/7/7/7 members = 57 sources) + 1 blob each for bakery/editor/game; byte-identical across two `generate --unity` runs; vendor TUs and `pch.cpp` keep standalone rules; plain `generate` restores per-TU file lists (0 unity references)

## 3. Unity build verification

- [x] 3.1 Unity clean build: **153.9 s**, 0 errors, 0 warnings (per-TU build on the same tree: 241.8 s → −36 %)
- [x] 3.2 Verified: `--frames=10` and `--frames=90 --screenshot=true` runs exit 0 with 0 OpenGL errors
- [x] 3.3 Runtime-project harness: per-TU 147.1 s vs unity 70.2 s (2.1x, ≤ 2/3 target met); full clean build 241.8 s → 153.9 s
- [x] 3.4 Incremental cost measured: touch 1 `.cpp` → 28.1 s (unity) vs 19.6 s (per-TU); recorded in the KB
- [x] 3.5 Verified exactly-once: 57 blob members + 8 standalone TUs (6 vendor + `opengl_imgui_build` + `pch.cpp`) = 65 sources; the link succeeded with no duplicate-symbol errors. Screenshot parity vs per-TU build: 99.93 % identical pixels (max delta 31/255)

## 4. Documentation

- [x] 4.1 `openspec/kb/build.md`: unity section added (opt-in flag, layout/exclusions, guidance, measured table incl. unity column)
- [x] 4.2 `openspec/kb/dev-scripts.md`: `generate --unity` documented

## 5. Final verification

- [x] 5.1 Default path: `generate` + clean build = 241.8 s, 0 errors / 0 warnings, no unity references in the Makefiles (no regression)
- [x] 5.2 Unity path: `generate --unity` + clean build = 153.9 s, 0 errors / 0 warnings (logs `/tmp/z1_unity_build2.log`, `/tmp/z1_default_build.log`)
- [x] 5.3 Runtime checks done for the unity binary (10 frames; 90-frame screenshot) with 0 OpenGL errors; the default binary was rebuilt and compared for parity afterwards
- [x] 5.4 `format --dry-run`: only the pre-existing `kinematic_platform.py` drift is reported (not touched); changed files are format-clean; experiment scratch files removed

## Evidence

| Scenario (reference machine, `-j8`) | per-TU | `--unity` |
|---|---|---|
| Clean Hybrid build of `game` | 241.8 s | 153.9 s (−36 %) |
| Runtime project (isolated harness) | 147.1 s | 70.2 s (2.1x) |
| Incremental: touch 1 `.cpp` | 19.6 s | 28.1 s |
| Warnings / errors | 0 / 0 | 0 / 0 |
| Runtime correctness | — | 0 GL errors, 99.93 % screenshot parity |

Logs: `/tmp/z1_unity_build2.log`, `/tmp/z1_default_build.log`, `/tmp/z1_unity_run.log`, `/tmp/z1_unity_shot.log`, `/tmp/z1_unity_results5.txt`.

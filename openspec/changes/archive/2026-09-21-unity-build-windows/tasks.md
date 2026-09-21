# Tasks: unity builds on Windows

## 1. Premake support

- [x] 1.1 Root `premake5.lua`: `--unity` option description no longer macOS-scoped; blob writer gained an optional
      `pch_name` argument that is emitted as the blob's first include
- [x] 1.2 Root `premake5.lua`: workspace-level `filter { "system:windows", "options:unity" } buildoptions { "/bigobj" }`
- [x] 1.3 Engine projects (runtime/editor/game/bakery): unity filter widened to
      `{ "system:macosx or system:windows", "options:unity" }`; runtime passes `"pch.h"`, the others have no PCH
- [x] 1.4 `dev/commands/generate.py`: `--unity` help/status text no longer says "macOS"

## 2. Pre-existing Profile-config fix

- [x] 2.1 `engine/runtime/premake5.lua`: PhysX `libdirs` filter `Release or Hybrid` → `Release or Profile or Hybrid`
      (PhysX ships Debug/Release only). Before: `LNK1181: cannot open input file 'PhysX_static_64.lib'` then 13x
      `LNK1104: runtime.lib`. After: `Build: 23 succeeded, 0 failed`

## 3. MSVC-specific fixes found while building

- [x] 3.1 PCH: every runtime blob failed with `C1010` — reproduced with no PCH include, with a path-qualified include,
      and with an include of a source file that includes `pch.h`; fixed by emitting `#include "pch.h"` first (the bare
      spelling MSVC matches against `/Yu`)
- [x] 3.2 Debug: `editor_unity_1.cpp` failed with `C1128: number of sections exceeded object file format limit` — fixed
      by `/bigobj` for unity builds on Windows

## 4. Unity build verification (Windows)

- [x] 4.1 `generate --unity` + `compile` succeeds for all four configurations, 23/23 projects, 0 errors / 0 warnings
- [x] 4.2 Blob composition verified: runtime 8 blobs / 57 sources, bakery/editor/game 1 blob each; 8 standalone TUs
      (6 vendor `_build.cpp`, `pch.cpp`, `imgui_layer.cpp`) keep individual compile rules; no `3rdparty` source appears
      in any blob
- [x] 4.3 Test results identical to the per-TU control in every configuration: Debug 13/13, Release 9/10,
      Profile 12/13, Hybrid 11/11 — the `test_render_graph` access violation in Release/Profile is pre-existing
      (reproduced in per-TU mode) and unrelated to unity
- [x] 4.4 `smoke --frames 120` against the unity-built binary: clean exit
- [x] 4.5 `generate --unity --probing` + Hybrid build: 0 errors (unity and probing compose; `generate` wipes the stale
      Hybrid objects as documented)

## 5. Benchmarks (Windows reference machine, clean builds, two runs per cell)

- [x] 5.1 per-TU: Debug 84.7 / 84.6 s, Release 138.9 / 136.7 s, Profile 126.0 / 127.4 s, Hybrid 142.0 / 142.9 s
- [x] 5.2 `--unity`: Debug 53.8 / 53.2 s, Release 86.0 / 86.0 s, Profile 75.1 / 75.1 s, Hybrid 86.6 / 87.9 s
      (1.58x – 1.69x faster)
- [x] 5.3 `.obj` totals: Debug 638 → 432 MB, Release 259 → 179 MB, Profile 317 → 225 MB, Hybrid 270 → 180 MB
- [x] 5.4 Incremental (Hybrid, touch one source): per-TU 6.9 s (no-op 1.6 s) vs unity 10.2 s

## 6. Documentation

- [x] 6.1 `openspec/kb/build.md`: unity section made platform-neutral, Windows protocol/numbers recorded, MSVC PCH and
      `/bigobj` requirements documented
- [x] 6.2 `openspec/kb/dev-scripts.md`: `--unity` described as macOS + Windows
- [x] 6.3 `openspec/specs/unity-build/spec.md`: platform-neutral requirements + MSVC constraints

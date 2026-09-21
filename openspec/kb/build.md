# Build
> Summary: How to generate, compile, format, test, and run z1engine
> Scope: dev/, utils/premake/, engine/bin/, premake5.lua

## Prerequisites

- Windows 10/11
- Visual Studio 2026 (Desktop C++ workload)
- Git
- Python 3.14 (bundled with project, or system install)

## Dev-Scripts CLI

All build commands go through `python dev/z1.py <command>`:

| Command | Purpose |
|---------|---------|
| `generate` | Regenerate VS project files via premake5 |
| `compile` | Build solution (`--config Debug\|Release\|Profile\|Hybrid`) |
| `format` | Format source code (tabs, whitespace, CRLF) |
| `validate-shaders` | Validate all GLSL shaders |
| `test` | Discover and run `test_*.exe` |
| `smoke` | Run editor smoke test (`--frames`) |
| `dcv` | Full develop-compile-verify loop |
| `release` | Package release folder |

-> see [dev-scripts.md]

## Quick Build

```cmd
python dev/z1.py generate
python dev/z1.py compile
```

## Build Configurations

| Config | Use |
|--------|-----|
| `Debug` | Development, assertions enabled, no optimization |
| `Release` | Optimized, for shipping |
| `Profile` | Optimized with profiling instrumentation |
| `Hybrid` | Optimized with asserts, checks, and debug symbols (breakpoints work) — default for dev CLI and run scripts |

## Output Paths

| Artifact | Path |
|----------|------|
| Editor | `engine/bin/Hybrid/editor.exe` |
| Game | `engine/bin/Hybrid/game.exe` |
| Tests | `engine/bin/test/Hybrid/test_*.exe` |
| Shader validator | `engine/bin/Hybrid/shader_validator.exe` |
| Intermediate | `engine/intermediate/` |

## Solution Structure

- Root `premake5.lua` defines all projects
- Test projects auto-discovered via `create_test()` iterating `engine/test/**.cpp`
- 3rdparty libraries linked via `engine/3rdparty/` include paths

## Premake Generation

```cmd
utils\premake\premake5.exe vs2022 --vs2026
```

- Must re-run when: new source files added/removed, `premake5.lua` modified
- Generates `z1engine.sln` at repo root

## VS2026 Paths

- Community: `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com`
- Professional/Enterprise: same base path with edition substituted

## macOS Builds

Apple clang + GNU make against premake `gmake` projects (`python dev/z1.py generate` runs `premake5 gmake`).
Outputs mirror Windows: `engine/bin/<Config>/`, objects in `engine/intermediate/<Config>/`.

| Command | Effect |
|---------|--------|
| `python dev/z1.py generate [--probing] [--full-symbols] [--unity]` | Regenerate Makefiles (prober / full debug info / unity opt-ins) |
| `python dev/z1.py compile [--config Hybrid] [--jobs N] [--ccache\|--no-ccache]` | Build the game target (`make config=<cfg> -j<jobs> game`) |
| `./compile.sh`, `./generate.sh` | Thin macOS wrappers around the dev CLI (no flag suppression) |

- **Jobs**: defaults to the logical CPU count (`--jobs N` overrides). The machine is usually CPU-throughput-bound,
  so more jobs than physical cores gains little (measured: `-j4` 361 s vs `-j8` 358 s on a 4C/8T 15 W CPU).
- **ccache**: used automatically when `ccache` is on `PATH` (`CC="ccache clang"`, `CXX="ccache clang++"`,
  `CCACHE_SLOPPINESS=pch_defines,time_macros` — required for the runtime precompiled header).
  `--no-ccache` disables it; `--ccache` fails fast when it is not installed. Install with `brew install ccache`.
- **Debug info**: Hybrid and Profile compile with `-gline-tables-only` (line-level breakpoints, ~3x smaller objects,
  faster codegen); Debug keeps full `-g`. `generate --full-symbols` restores full `-g` for Hybrid/Profile.
- **Warnings**: engine projects (runtime/editor/game/bakery) compile with `-Wall -Wextra -Wno-unused-parameter`
  on macOS and the tree is warning-free. `engine/3rdparty/` is never edited; the vendored `stb_build.cpp` units
  scope two vendored diagnostics through a per-file premake filter. The reflection macros carry the one allowed
  clang diagnostic push/pop (`REFLECT_OFFSETOF_DIAG_*` in `core/core.h`, also used by manual registrations).
- **Flag changes do not invalidate objects**: `make` cannot see premake option changes, so wipe
  `engine/intermediate/<Config>` (and `engine/bin/<Config>`) after changing build flags — `generate --probing`
  does this automatically for its own toggle.

### Unity builds (opt-in, macOS)

`python dev/z1.py generate --unity` generates deterministic unity blob translation units for the engine projects
(runtime/editor/game/bakery) and builds those instead of the individual sources. premake's own `enableunitybuild`
setting only exists for VS actions, so the blobs are written by `unity_blob_project()` in the root `premake5.lua`.

- Layout: `engine/intermediate/unity/<project>/<project>_unity_<n>.cpp` (gitignored, regenerated on every
  `generate --unity`; run it again after adding/removing sources). Sources are sorted and split round-robin into
  groups of 8, so heavy TUs (`py_engine*`, `reflection_hooks`) spread across blobs.
- Exclusions: `source/3rdparty/*.cpp`, any `*_build.cpp` (stb/tinyexr/tinygltf/tinyobjloader/imgui/
  `opengl_imgui_build`) and `pch.cpp` stay standalone — they define vendor implementations/loaders that conflict
  inside a blob. Everything else is compiled exactly once through its blob.
- Use it for cold/CI builds; keep the default per-TU build for interactive development (editing one source
  recompiles its whole blob).
- On memory-constrained machines lower the job count (`--jobs 4`): several blobs at `-O2` are memory hungry.

### Measured baselines (reference machine: Intel i5-8257U 4C/8T 15 W, 8 GB, Apple clang 17)

Protocol: `--clean` = remove `engine/intermediate/Hybrid` + `engine/bin/Hybrid`; timings from the dev CLI RESULT line.

| Scenario | Before all changes | After (per-TU) | After (`--unity`) |
|----------|--------------------|----------------|-------------------|
| Clean Hybrid build of `game` (-j8) | 361.5 s (fresh) / 333 s (warm machine) | 241.8 s | **153.9 s** |
| Clean runtime project (isolated harness) | 268 s | 147.1 s | 70.2 s |
| Incremental: touch 1 `.cpp` | 25.1 s | 19.6 s | 28.1 s |
| Incremental: touch `render/global.h` (wide cascade) | 181 s | 133.7 s | — |
| No-op build (everything up to date) | — | 1.4 s | — |
| `runtime` object directory | 307 MB | 106 MB | 106 MB |
| Warnings (full build) | 1619 | 0 | 0 |
| `libruntime.a` | — | 59 MB | ~59 MB |

Correctness for the unity mode: binary runs `--frames=10` and `--frames=90 --screenshot=true` with 0 OpenGL errors;
screenshot parity against the per-TU build is 99.93 % identical pixels (max channel delta 31/255, TAA/jitter noise
at edges).

Per-TU profiling note: backend (codegen/dwarf) accounted for ~67 % of TU time before the debug-info change —
that is why the debug-info level was the dominant lever, not include hygiene. The `GLFW_INCLUDE_NONE` spelling fix
(2026-09-21) additionally stopped TUs that include `glfw3.h` from parsing Apple's legacy `<OpenGL/gl.h>`
(per-TU runtime project 167 s → 147 s in the harness).

## Troubleshooting

- **"premake5 not recognized"**: use `utils\premake\premake5.exe` directly
- **Missing DLLs**: ensure post-build steps copied `python314.dll` to output dir
- **Solution not found**: run `python dev/z1.py generate` first

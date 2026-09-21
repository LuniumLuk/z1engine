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
| `python dev/z1.py generate [--probing] [--full-symbols]` | Regenerate Makefiles (prober / full debug info opt-ins) |
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

### Measured baselines (reference machine: Intel i5-8257U 4C/8T 15 W, 8 GB, Apple clang 17)

Protocol: `--clean` = remove `engine/intermediate/Hybrid` + `engine/bin/Hybrid`; timings from the dev CLI RESULT line.

| Scenario | Before | After |
|----------|--------|-------|
| Clean Hybrid build of `game` (-j8) | 361.5 s (fresh) / 333 s (warm machine) | 250.1 s |
| Clean runtime project only | 268 s | 193 s (line tables) |
| Incremental: touch 1 `.cpp` | 25.1 s | 18.9 s |
| Incremental: touch `render/global.h` (wide cascade) | 181 s | 133.7 s |
| No-op build (everything up to date) | — | 1.4 s |
| `runtime` object directory | 307 MB | 106 MB |
| Warnings (full build) | 1619 | 0 |
| `libruntime.a` | — | 59 MB |

Per-TU profiling note: backend (codegen/dwarf) accounted for ~67 % of TU time before the debug-info change —
that is why the debug-info level was the dominant lever, not include hygiene.

## Troubleshooting

- **"premake5 not recognized"**: use `utils\premake\premake5.exe` directly
- **Missing DLLs**: ensure post-build steps copied `python314.dll` to output dir
- **Solution not found**: run `python dev/z1.py generate` first

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
| `compile` | Build solution (`--config Debug\|Release\|Profile`) |
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

## Output Paths

| Artifact | Path |
|----------|------|
| Editor | `engine/bin/Debug/editor.exe` |
| Game | `engine/bin/Debug/game.exe` |
| Tests | `engine/bin/test/Debug/test_*.exe` |
| Shader validator | `engine/bin/Debug/shader_validator.exe` |
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

## Troubleshooting

- **"premake5 not recognized"**: use `utils\premake\premake5.exe` directly
- **Missing DLLs**: ensure post-build steps copied `python314.dll` to output dir
- **Solution not found**: run `python dev/z1.py generate` first

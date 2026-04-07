# Project Map
> Summary: Directory tree, key files, and file naming patterns for z1engine
> Scope: D:\z1engine\main\

## Top-Level Directories

| Directory | Purpose |
|-----------|---------|
| `asset/` | Source assets used by the project |
| `content/` | Runtime content (scenes, configs) |
| `dev/` | Build scripts and dev CLI (`z1.py`) |
| `docs/` | Documentation (deprecated, migrated to KB) |
| `engine/` | Core engine code, 3rdparty, binaries |
| `openspec/` | OpenSpec workflow: specs, KB, changes |
| `utils/` | Helper tools (premake binary, format_code.py) |
| `.opencode/` | OpenCode agent command definitions |
| `.github/` | GitHub prompts and workflows |

## Engine Subdirectories

| Directory | Purpose |
|-----------|---------|
| `engine/3rdparty/` | Bundled libraries (glfw, yaml-cpp, stb, tinyexr, lz4, imgui, entt, glm) |
| `engine/bakery/` | Offline asset processing tool |
| `engine/bin/` | Build output (executables, DLLs) |
| `engine/config/` | Engine configuration files |
| `engine/content/` | Runtime content (shaders, scripts, scenes) |
| `engine/editor/` | Editor application source |
| `engine/game/` | Standalone game executable source |
| `engine/intermediate/` | Intermediate build artifacts |
| `engine/runtime/` | Core runtime library source |
| `engine/stubs/` | Stub projects |
| `engine/test/` | Test source files (`test_*.cpp`) |
| `engine/tool/` | Tool projects (shader_validator) |

## Runtime Modules (`engine/runtime/source/`)

| Module | Key Files |
|--------|-----------|
| `core/` | `application.h`, `window.h`, `layer.h`, `log.h`, `core.h` |
| `scene/` | `scene.h`, `entity.h`, `component/*.h`, `*_system.h` |
| `render/` | `render_graph.h`, `shader.h`, `framebuffer.h`, `renderer/*.h`, `rhi/*.h` |
| `asset/` | `asset.h`, `asset_manager.h`, `binary_file.h`, `importer/*.h` |
| `event/` | Event dispatch system |
| `python/` | `python_layer.cpp`, `py_engine.cpp`, `python_script.cpp` |
| `animation/` | Skeletal animation system |
| `util/` | Math, profiling, reflection utilities |

## Key Root Files

| File | Purpose |
|------|---------|
| `premake5.lua` | Build system definition, project generation |
| `z1engine.sln` | VS2026 solution (generated) |
| `run_editor.bat` | Launch editor |
| `run_game.bat` | Launch game |
| `validate_shaders.bat` | Thin wrapper for shader validation |

## File Naming Conventions

- C++ source: `lowercase_underscore.cpp` / `.h`
- Tests: `test_<name>.cpp` in `engine/test/`
- Shaders: `<name>.glsl` in `engine/content/shader/`
- Shader includes: `engine/content/shader/include/`
- Python scripts: `<name>.py` in `content/scripts/`

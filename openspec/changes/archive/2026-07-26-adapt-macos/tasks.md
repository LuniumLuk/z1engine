## 1. Third-Party Library Preparation

- [ ] 1.1 Source premake5 macOS binary and place at `utils/premake/premake5` (or install via Homebrew: `brew install premake`)
- [ ] 1.2 Clone Native File Dialog Extended (NFD) into `engine/3rdparty/nfd/`:
  - Source: `https://github.com/btzy/nativefiledialog-extended`
  - Add `nfd.h`, `nfd_common.c`, `nfd_cocoa.m` to the project
  - Create `engine/3rdparty/nfd/premake5.lua`
  - Add `include "engine/3rdparty/nfd"` to root `premake5.lua` under the dependency group
- [ ] 1.3 Set up Python 3.14 via pyenv on macOS:
  - `pyenv install 3.14.0` (or latest 3.14.x)
  - `pyenv local 3.14.0` in the repo root
  - Verify: `python3 --version` reports 3.14.x
  - Note: Python headers and dylib paths will be auto-detected by the build system

## 2. Build System — Premake5 macOS Filters

- [ ] 2.1 Update root `premake5.lua`:
  - Add `filter "system:macosx"` block that defines `PLATFORM_MACOS` for all projects
  - Guard `toolset "v145"` filter behind `system:windows`
  - Guard `postbuildcommands` with `.dll` copies behind `system:windows`
- [ ] 2.2 Update `engine/runtime/premake5.lua`:
  - Add `filter "system:macosx"` with `PLATFORM_MACOS`, remove `BUILD_DLL` define
  - Replace `opengl32.lib` with OpenGL framework under macOS filter
  - Guard MSVC-specific flags (`/IGNORE:4006`, `/bigobj`) behind `system:windows`
  - Guard PhysX links, libdirs, and `PX_PHYSX_STATIC_LIB` define behind `system:windows`
  - Add NFD to links and includedirs
  - Add `-framework Cocoa -framework IOKit -framework CoreVideo` under macOS filter
- [ ] 2.3 Update `engine/editor/premake5.lua`:
  - Add `filter "system:macosx"` with `PLATFORM_MACOS`
  - Link NFD
- [ ] 2.4 Update `engine/game/premake5.lua`:
  - Add `filter "system:macosx"` with `PLATFORM_MACOS`
  - Guard `python314.dll`/`.zip` postbuild copy behind `system:windows`
  - Add macOS Python dylib/framework copy under `system:macosx`
- [ ] 2.5 Update `engine/3rdparty/glfw/premake5.lua`:
  - Enable the existing `filter "system:macosx"` block (source files are already listed)
  - Add macOS system frameworks to links: `Cocoa`, `IOKit`, `CoreVideo`
- [ ] 2.6 Update `engine/3rdparty/glad/premake5.lua`: add `filter "system:macosx"` (no special settings needed)
- [ ] 2.7 Update `engine/3rdparty/imgui/premake5.lua`: add `filter "system:macosx"`
- [ ] 2.8 Update `engine/3rdparty/imguizmo/premake5.lua`: add `filter "system:macosx"`
- [ ] 2.9 Update `engine/3rdparty/lz4/premake5.lua`: add `filter "system:macosx"`
- [ ] 2.10 Update `engine/3rdparty/yaml-cpp/premake5.lua`: add `filter "system:macosx"`
- [ ] 2.11 Update `engine/bakery/premake5.lua`: add `filter "system:macosx"`
- [ ] 2.12 Update `engine/tool/shader_validator/premake5.lua`:
  - Replace `opengl32.lib` with `-framework OpenGL` under macOS filter
  - Add `filter "system:macosx"`
- [ ] 2.13 Update `engine/tool/importer/premake5.lua`:
  - Add `filter "system:macosx"` with `PLATFORM_MACOS`
  - Guard `python314.dll` postbuild copy and Windows-only settings behind `system:windows`
  - Note: importer will not build on macOS initially — premake filter just ensures clean generation
- [ ] 2.14 Verify premake5 generation produces valid Makefile on macOS (`python3 dev/z1.py generate`)

## 3. C++ Platform Layer

- [ ] 3.1 Update `engine/runtime/source/core/core.h`:
  - Remove the `#error z1engine only support windows platform!` guard
  - Add `#ifdef PLATFORM_MACOS` branch for `API` macro (define as empty for static libs)
- [ ] 3.2 Update `engine/runtime/source/pch.h`:
  - Guard `#include <Windows.h>` with `#ifdef PLATFORM_WINDOWS`
  - Guard MSVC `#pragma warning(disable: 4996)` with `#ifdef _MSC_VER`
- [ ] 3.3 Update `engine/editor/source/gui.cpp`:
  - Replace `#include <windows.h>` and `#include <commdlg.h>` with `#include <nfd.h>`
  - Rewrite `open_file_dialog()` and `save_file_dialog()` to use `NFD_OpenDialog()` / `NFD_SaveDialog()`
  - Remove `GLFW_EXPOSE_NATIVE_WIN32` and `glfwGetWin32Window()` usage
- [ ] 3.4 Update `engine/runtime/source/python/python_layer.cpp`:
  - Replace `#include <conio.h>` with `#ifdef PLATFORM_WINDOWS` guard
  - Add `#ifdef PLATFORM_MACOS` implementation of `_kbhit()` and `_getch()` using `<termios.h>`, `<fcntl.h>`, `<unistd.h>`
- [ ] 3.5 Update `engine/runtime/source/core/io.cpp`:
  - Guard the Windows reserved device name check (`CON`, `PRN`, etc.) with `#ifdef PLATFORM_WINDOWS`
- [ ] 3.6 Update `engine/runtime/source/core/window.cpp`:
  - Add macOS OpenGL context hints under `#ifdef PLATFORM_MACOS`: `GLFW_OPENGL_PROFILE` = `GLFW_OPENGL_CORE_PROFILE`, `GLFW_OPENGL_FORWARD_COMPAT` = `GLFW_TRUE`, version 4.1
- [ ] 3.7 Disable PhysX on macOS:
  - In `engine/runtime/source/core/core.h`: guard `PhysicsSystem` forward declaration and `m_physics_system` member with `#ifdef PLATFORM_WINDOWS`
  - In `engine/runtime/source/core/core.cpp`: guard PhysX init/shutdown with `#ifdef PLATFORM_WINDOWS`
  - In `engine/runtime/source/scene/scene.cpp`: guard `PhysicsSystem::update` call with `#ifdef PLATFORM_WINDOWS`
  - In `engine/runtime/source/core/reflection_hooks.cpp`: guard `ColliderComponent` and `PhysicsComponent` registration with `#ifdef PLATFORM_WINDOWS`
- [ ] 3.8 Audit renderer for OpenGL 4.2+ API usage:
  - Search for DSA functions: `glCreateTextures`, `glTextureSubImage2D`, `glTextureParameteri`, `glCreateBuffers`, `glNamedBufferData`, etc.
  - Gate DSA calls behind `#ifdef PLATFORM_WINDOWS` with non-DSA fallback paths (`glGenTextures` + `glBindTexture` + `glTexImage2D`, etc.)

## 4. Shader Validator — OpenGL 4.1 Adaptation

- [ ] 4.1 Update `engine/tool/shader_validator/main.cpp`:
  - On macOS (`#ifdef PLATFORM_MACOS`), request OpenGL 4.1 core profile context via GLFW hints (same pattern as window.cpp)
  - Audit shader compilation for GLSL features beyond 4.1 (e.g., `#version 460` → `#version 410 core`)
  - Add version-gated shader compilation if needed
- [ ] 4.2 Verify all shaders in `engine/content/shader/` compile on OpenGL 4.1:
  - Check `#version` directives — downgrade or add `#ifdef` alternatives where needed
  - Test: `python3 dev/z1.py validate-shaders` on macOS

## 5. Python Dev CLI — Platform Awareness

- [ ] 5.1 Add platform detection to `dev/commands/_common.py`:
  - `is_macos()` / `is_windows()` helpers
  - `get_executable_extension()` → `".exe"` on Windows, `""` on macOS
  - `get_shared_lib_extension()` → `".dll"` on Windows, `".dylib"` on macOS
  - `get_premake5_path()` → platform-appropriate binary discovery (PATH first, then `utils/premake/`)
  - `find_make()` → check for `make` on PATH
- [ ] 5.2 Update `dev/commands/generate.py`:
  - Use `get_premake5_path()` instead of hardcoded `.exe` path
  - On macOS, invoke `premake5 gmake2`
  - Keep existing Windows behavior (`premake5 vs2022 --vs2026`) unchanged
- [ ] 5.3 Update `dev/commands/compile.py`:
  - On macOS: invoke `make` with config, parse GCC/Clang errors (add Clang error regex pattern)
  - Keep existing `devenv.com` behavior for Windows unchanged
- [ ] 5.4 Update `dev/commands/test.py`:
  - On macOS: discover executables without `.exe` extension, use `os.access(x, os.X_OK)` to check executability
  - On Windows: keep existing `test_*.exe` discovery
- [ ] 5.5 Update `dev/commands/smoke.py`:
  - Use `get_executable_extension()` for candidate paths — search for `game` / `editor` without hardcoded `.exe`
- [ ] 5.6 Update `dev/commands/release.py`:
  - Use `get_executable_extension()` and `get_shared_lib_extension()` for file copies
- [ ] 5.7 Update `dev/commands/validate_shaders.py`:
  - Use `get_executable_extension()` for `shader_validator` path
- [ ] 5.8 Update `dev/commands/format_cmd.py`:
  - On macOS: enforce LF line endings instead of CRLF

## 6. Shell Scripts

- [ ] 6.1 Create `generate.sh` — invokes `python3 dev/z1.py generate`
- [ ] 6.2 Create `compile.sh` — invokes `python3 dev/z1.py compile`
- [ ] 6.3 Create `run_editor.sh` — invokes `./engine/bin/Debug/editor`
- [ ] 6.4 Create `run_game.sh` — invokes `./engine/bin/Debug/game`
- [ ] 6.5 Create `validate_shaders.sh` — invokes `python3 dev/z1.py validate-shaders`

## 7. Build Validation

- [ ] 7.1 Run `python3 dev/z1.py generate` on macOS — verify Makefile is produced without errors
- [ ] 7.2 Run `python3 dev/z1.py compile` on macOS — verify the engine builds to completion (or identify and fix remaining compilation errors)
- [ ] 7.3 Run `python3 dev/z1.py test` on macOS — verify test discovery works and tests pass
- [ ] 7.4 Run `python3 dev/z1.py smoke --frames 5` on macOS — verify the editor/game launches and renders without crash
- [ ] 7.5 Verify Windows build is not broken: run `generate` + `compile` on Windows

- [ ] 7.1 Run `python dev/z1.py generate` on macOS — verify Makefile (or Xcode project) is produced without errors
- [ ] 7.2 Run `python dev/z1.py compile` on macOS — verify the engine builds to completion (or identify and fix remaining compilation errors)
- [ ] 7.3 Run `python dev/z1.py test` on macOS — verify test discovery works and tests pass
- [ ] 7.4 Run `python dev/z1.py smoke --frames 5` on macOS — verify the editor/game launches and renders without crash
- [ ] 7.5 Verify Windows build is not broken: run `generate` + `compile` on Windows

## Why

z1engine is currently locked to Windows x64 with Visual Studio 2026. Every layer of the engine — the build system (Premake5 → VS solutions), the C++ source (PLATFORM_WINDOWS guards, Win32 API calls, `<conio.h>`), the Python dev CLI (`devenv.com`, `.exe` paths, `find_vs2026()`), and the shell scripts (`.bat` files) — assumes Windows. Adapting to macOS unlocks development on Apple Silicon Macs, expands the contributor base, and enables macOS-native builds of the editor and game.

## What Changes

- **Build system (Premake5)** — Add `filter "system:macosx"` blocks alongside existing `system:windows` blocks across all `premake5.lua` files. Generate Xcode projects or Makefiles on macOS instead of Visual Studio solutions. Replace Windows-specific linker flags (`opengl32.lib`, `/IGNORE:4006`, `/bigobj`) with macOS equivalents (`-framework OpenGL`, `-rpath`, etc.).
- **C++ platform layer** — Introduce `PLATFORM_MACOS` define. Replace `<windows.h>` usage in `pch.h` with platform-agnostic headers or conditional includes. Replace Win32 file dialogs (`OPENFILENAMEA` in `gui.cpp`) with native macOS equivalents via Cocoa or a cross-platform library like NFD. Replace `<conio.h>` (`_kbhit`/`_getch` in `python_layer.cpp`) with POSIX `termios` equivalents. Remove the hard `#error` in `core.h` for non-Windows platforms.
- **Python dev CLI** — Make `dev/z1.py` commands platform-aware: detect Premake5 binary by OS (`.exe` vs no extension), replace `devenv.com` compilation with `xcodebuild` or `make`, replace `.exe`/`.dll` path assumptions with macOS equivalents (no extension/`.dylib`), implement `find_xcode()` or `find_make()` alongside `find_vs2026()`.
- **Shell scripts** — Create `.sh` equivalents for all `.bat` convenience scripts.
- **Third-party libraries** — Build PhysX 5.6 from source for macOS (arm64 + x86_64). Source Python 3.14 framework or `.dylib` for macOS. GLFW and GLAD already have macOS support in their premake config; just enable the `system:macosx` filter.
- **Rendering** — Switch from `opengl32.lib` to `-framework OpenGL`. GLAD + GLFW handle the rest. Ensure OpenGL 4.1+ context creation (macOS caps at 4.1 without Metal/ANGLE).

## Capabilities

### New Capabilities

- `macos-build-system`: Premake5 generation for Xcode or Makefiles on macOS, with `filter "system:macosx"` blocks across all projects. Platform-appropriate linker flags, frameworks, and defines.
- `macos-platform-layer`: `PLATFORM_MACOS` preprocessor define, platform-agnostic windowing (GLFW), native file dialogs replacing Win32 COMMDLG, POSIX terminal I/O replacing `<conio.h>`, and the removal of the Windows-only `#error` guard.
- `macos-dev-scripts`: Cross-platform Python dev CLI that detects the OS, selects the correct Premake5 binary, build tool (xcodebuild/make), and executable extensions.
- `macos-thirdparty`: macOS builds of PhysX 5.6 (arm64 + x86_64 universal), macOS Python 3.14 embedded distribution, and OpenGL framework linking.

### Modified Capabilities

- `dev-scripts`: Extend the dev CLI requirement to support `macos` as a platform. Commands `generate`, `compile`, `test`, `smoke`, `release`, and `validate-shaders` must work on macOS with platform-appropriate tooling (xcodebuild/make instead of devenv.com, no `.exe` extension assumptions).

## Impact

- **All `premake5.lua` files** (root + `engine/runtime/`, `engine/editor/`, `engine/game/`, `engine/bakery/`, `engine/3rdparty/*/`, `engine/tool/*/`) — add `filter "system:macosx"` blocks
- `engine/runtime/source/core/core.h` — remove `#error` guard, add `PLATFORM_MACOS` branch for `API` macro
- `engine/runtime/source/pch.h` — conditional `<Windows.h>` include
- `engine/editor/source/gui.cpp` — replace Win32 file dialogs with cross-platform alternative
- `engine/runtime/source/python/python_layer.cpp` — replace `<conio.h>` with POSIX terminal I/O
- `engine/runtime/source/core/io.cpp` — make Windows-reserved-name check Windows-only (macOS doesn't have these restrictions)
- `dev/z1.py` — add platform detection module
- `dev/commands/_common.py` — add `find_xcode()`/`find_make()`, platform-aware Premake5 path
- `dev/commands/generate.py` — platform-aware premake invocation
- `dev/commands/compile.py` — xcodebuild/make build support
- `dev/commands/test.py` — platform-aware executable discovery
- `dev/commands/smoke.py` — platform-aware executable path
- `dev/commands/release.py` — platform-aware binary packaging
- `dev/commands/validate_shaders.py` — platform-aware executable path
- All `.bat` files — create `.sh` equivalents
- `engine/3rdparty/physx/` — macOS library placement
- `engine/3rdparty/python314/` — macOS Python distribution

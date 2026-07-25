## Context

z1engine is a C++17 game engine targeting Windows x64 exclusively. Every layer assumes Windows:

| Layer | Windows assumption |
|-------|-------------------|
| **Premake5** | All `filter "system:windows"` blocks, no macOS filters. Root `premake5.lua` generates VS2022 solutions with `--vs2026` flag. |
| **C++ source** | `core.h` has `#error` for non-Windows. `pch.h` includes `<Windows.h>`. `gui.cpp` uses Win32 `OPENFILENAMEA` for file dialogs. `python_layer.cpp` uses `<conio.h>` for `_kbhit`/`_getch`. |
| **Linker** | `opengl32.lib`, `/IGNORE:4006`, `/bigobj` — all MSVC-specific. |
| **Python CLI** | `_common.py` searches `C:\Program Files\Microsoft Visual Studio\18\` for VS2026. `compile.py` invokes `devenv.com`. All commands assume `.exe` and `.dll` extensions. |
| **Shell scripts** | `.bat` files only; no `.sh` equivalents. |
| **Third-party** | PhysX static libs placed as Windows `.lib` files. Python 3.14 placed as `python314.dll`/`.lib`. |
| **Rendering** | OpenGL 4.6 via GLAD on Windows. macOS caps at OpenGL 4.1 (core profile only). |

The engine already uses cross-platform libraries (GLFW, GLAD, ImGui, EnTT, glm, spdlog, yaml-cpp, stb, pybind11). The Windows lock-in is concentrated in the build system, a handful of C++ source files, the Python dev CLI, and the PhysX/Python third-party placement.

## Goals / Non-Goals

**Goals:**
- Build and run the editor and game executables on macOS (Apple Silicon + Intel)
- Use Premake5 to generate either Xcode project files or Makefiles
- Compile with Apple Clang (the system compiler on macOS)
- Link against macOS system frameworks (OpenGL, Cocoa, IOKit, CoreVideo)
- Replace Win32-specific C++ code with cross-platform or macOS-native equivalents
- Make the Python dev CLI (`dev/z1.py`) fully functional on macOS
- Provide `.sh` equivalents for all `.bat` convenience scripts
- Source and integrate macOS-compatible PhysX 5.6 libraries
- Source macOS Python 3.14 embedded distribution
- Maintain full backward compatibility with the existing Windows build

**Non-Goals:**
- Metal render backend — OpenGL 4.1 core profile is sufficient for parity
- Vulkan render backend
- Linux support (though the PLATFORM_MACOS changes pave the way)
- ARM64-specific SIMD optimizations
- Universal binary fat builds — separate arch builds are acceptable initially
- Notarization or code signing for macOS distribution
- Homebrew formula or macOS installer package
- Replacing GLFW with a native Cocoa window — GLFW already handles Cocoa on macOS

## Decisions

### Decision 1: Premake5 generates Makefiles on macOS

**Decision**: On macOS, `dev/z1.py generate` invokes `premake5 gmake2` (GNU Makefiles). No Xcode project generation is supported — the project's workflow is entirely terminal/CLI-driven, and Makefiles serve that perfectly.

**Rationale**: Makefiles are simple, fast to generate, universally available on macOS (via Xcode Command Line Tools), and integrate naturally with the `dev/z1.py` CLI workflow. Xcode project generation adds complexity (scheme management, index store) with no benefit for a terminal-oriented development team.

**Alternatives considered**:
- *Xcode project generation*: Rejected — unnecessary complexity for a CLI-first project.
- *CMake instead of Premake5*: Rejected — would require rewriting all build definitions. Out of scope.

---

### Decision 2: Single PLATFORM_MACOS define, no PLATFORM_UNIX abstraction

**Decision**: Add `PLATFORM_MACOS` as a new preprocessor define alongside `PLATFORM_WINDOWS`. Do NOT introduce a `PLATFORM_UNIX` or `PLATFORM_POSIX` abstraction layer at this stage.

**Rationale**: A `PLATFORM_UNIX` abstraction would be premature. We have no Linux target to validate against. Adding `PLATFORM_MACOS` directly keeps the change minimal and readable. If Linux support is added later, common Unix code can be factored into `PLATFORM_UNIX` at that point.

The `API` macro (`__declspec(dllexport/dllimport)`) in `core.h` will map to empty on macOS (symbol visibility is controlled by `-fvisibility=hidden` and `__attribute__((visibility("default")))` if needed).

---

### Decision 3: Replace Win32 file dialogs with NFD (Native File Dialog)

**Decision**: Add the [Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended) library (single-header MIT license) to replace `OPENFILENAMEA` in `gui.cpp`. Wrap calls in platform-agnostic helper functions.

**Rationale**: NFD provides native open/save dialogs on Windows, macOS, and Linux via a simple C API. It's a single header + one `.c` implementation, fits the project's 3rdparty pattern perfectly. Calling Cocoa APIs directly from C++ would require Objective-C++ compilation units and significantly more code.

**Alternatives considered**:
- *Tiny file dialog*: Similar, but fewer features, less maintained.
- *Direct Cocoa API via Objective-C++*: Rejected — adds a new language to the build, more complex.
- *ImGui file dialog*: Rejected — not native look-and-feel, worse UX.

---

### Decision 4: POSIX termios replaces `<conio.h>` for PythonLayer

**Decision**: Replace `_kbhit()` and `_getch()` in `python_layer.cpp` with POSIX `termios` + `fcntl` equivalents, gated by `#ifdef PLATFORM_MACOS`.

**Rationale**: The console I/O is only needed for the embedded Python REPL to check for keyboard input without blocking. POSIX `termios` is the standard way to do non-blocking character input on Unix systems. The change is ~15 lines and fully self-contained.

---

### Decision 5: `make` is the sole macOS build tool

**Decision**: The `compile` dev command invokes `make` on macOS, using the Makefiles generated by `gmake2`. No `xcodebuild` support — make is simple, universal (available via Xcode Command Line Tools without the full Xcode IDE), and sufficient for this project's needs.

**Rationale**: `make` is universally available on macOS, works with the gmake2 Premake5 output, and integrates cleanly with the CLI. Adding `xcodebuild` support would require Xcode project generation (rejected in Decision 1) and adds unnecessary complexity.

---

### Decision 6: PhysX disabled on macOS (macro guard)

**Decision**: PhysX integration is disabled on macOS via a `#ifdef PLATFORM_WINDOWS` macro guard. `PhysicsSystem`, `PhysicsComponent`, and `ColliderComponent` are compiled out on macOS builds. PhysX libraries are not linked on macOS.

**Rationale**: Building PhysX 5.6 from source for macOS adds significant complexity (CMake toolchain setup, potential Apple Clang compatibility issues). Physics is not critical for the initial macOS port — the primary use case is editor-based scene editing and rendering. Physics can be re-enabled in a follow-up change if needed.

**Implementation**: In `engine/runtime/premake5.lua`, PhysX links and libdirs are gated behind `filter "system:windows"`. In C++ source, `#ifdef PLATFORM_WINDOWS` guards around PhysX includes and system registration.

---

### Decision 7: Python 3.14 managed via pyenv

**Decision**: Python 3.14 on macOS is managed via [pyenv](https://github.com/pyenv/pyenv). The engine's `get_embedded_python_home()` auto-detects the active pyenv Python installation. For distribution, a bundled Python framework can be placed at `engine/3rdparty/python314/` as a fallback.

**Rationale**: pyenv is the standard way to manage multiple Python versions on macOS. It allows developers to install and switch between Python versions without conflicting with the system Python. The engine already searches multiple locations for Python in `get_embedded_python_home()` — adding pyenv's `~/.pyenv/versions/` paths is a natural extension.

The `.dylib`/framework is linked instead of `.dll`/`.lib`. Python standard library paths are also macOS-appropriate (e.g., `lib/python3.14/`).

---

### Decision 8: OpenGL 4.1 Core Profile on macOS with macro guards

**Decision**: On macOS, request an OpenGL 4.1 core profile context via GLFW hints (`GLFW_OPENGL_PROFILE_CORE`, `GLFW_OPENGL_FORWARD_COMPAT`). Any OpenGL 4.2+ features used on Windows (e.g., DSA functions like `glCreateTextures`) are gated behind `#ifdef PLATFORM_WINDOWS` with non-DSA fallback paths for macOS. The existing OpenGL 4.6 code path remains for Windows.

**Rationale**: Apple caps OpenGL at 4.1 core profile. The engine does not currently use compute shaders, which are available in 4.1. The main gap is DSA (Direct State Access, OpenGL 4.5+), which provides convenience wrappers for texture/buffer management — all DSA calls have equivalent non-DSA fallbacks (`glGenTextures` + `glBindTexture` + `glTexImage2D`). Macro guards keep the Windows path using DSA for better performance while macOS uses the compatible fallback.

---

### Decision 9: Build architecture — x86_64 (Intel Mac)

**Decision**: The initial macOS port targets `x86_64` (Intel Mac). Apple Silicon (`arm64`) support can be added later as a follow-up. No universal binary builds.

**Rationale**: The development system is an Intel Mac. Targeting the host architecture avoids cross-compilation complexity. Once the port is stable on x86_64, arm64 support is a matter of adding the architecture to the Premake5 config and rebuilding — no source changes should be needed since both are 64-bit little-endian platforms using Apple Clang.

---

## Risks / Trade-offs

- **[Risk] OpenGL 4.1 feature gap** — The engine may use OpenGL features unavailable in 4.1 core profile (e.g., `glCreateTextures` DSA). No compute shaders are currently used, so that gap is not a concern.  
  → **Mitigation**: Audit the renderer for 4.2+ API usage during implementation. Gate DSA calls behind `#ifdef PLATFORM_WINDOWS` with non-DSA fallback paths for macOS. All DSA usage has equivalent pre-4.5 alternatives.

- **[Risk] Python 3.14 macOS availability** — Python 3.14 is very new; official macOS installers may not exist yet.  
  → **Mitigation**: Python 3.14 is managed via pyenv, which can build CPython from source on macOS. The engine can also fall back to Python 3.12/3.13 with minor compatibility shims if 3.14 is unavailable.

- **[Risk] Premake5 macOS binary** — The project bundles `utils/premake/premake5.exe` (Windows). A macOS premake5 binary must be sourced.  
  → **Mitigation**: Premake5 provides official macOS binaries. The dev CLI will search for a system-installed `premake5` first, then fall back to `utils/premake/premake5` (the macOS binary placed by the developer).

- **[Risk] ImGui GLFW backend macOS mouse coordinate bug** — GLFW 3.3.1 had a macOS window repositioning bug on resize (fixed in GLFW 3.3.1+).  
  → **Mitigation**: GLFW is already pinned at a recent version in the project. If issues arise, update the GLFW submodule to the latest 3.4.x release.

- **[Trade-off] PhysX unavailable on macOS** — Physics simulation is disabled on macOS builds. Entities with `PhysicsComponent` or `ColliderComponent` will not have physics behavior.  
  → This is acceptable for the initial port. The primary macOS use case is editor-based scene editing and rendering. Physics can be re-enabled in a follow-up if needed.

## Open Questions

1. **Should we add a CI pipeline for macOS builds?** — GitHub Actions offers macOS runners (Intel and M1). Adding a macOS CI job would prevent regressions. This is a follow-up concern, not part of the initial port.

2. **Shader validator OpenGL 4.1 adaptation** — The shader validator must be adapted for the OpenGL 4.1 core profile subset on macOS. Some GLSL features may need version-gated alternatives. The validator will be modified to use 4.1-compatible shader compilation on macOS while keeping the existing path on Windows.

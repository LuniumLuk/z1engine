## ADDED Requirements

### Requirement: Python 3.14 is managed via pyenv on macOS

Python 3.14 on macOS SHALL be managed via pyenv. The engine's `get_embedded_python_home()` SHALL detect pyenv-managed Python installations. A bundled Python framework at `engine/3rdparty/python314/` SHALL serve as a fallback for distribution.

#### Scenario: Python macOS library exists
- **WHEN** a developer sets up Python 3.14 via pyenv
- **THEN** a Python shared library (`.dylib` or framework) SHALL exist in the pyenv version directory
- **AND** Python headers SHALL be available at the pyenv include path
- **AND** the Python standard library SHALL be available

#### Scenario: Python home detection on macOS
- **WHEN** `get_embedded_python_home()` is called on macOS
- **THEN** it SHALL search pyenv version paths (e.g., `~/.pyenv/versions/3.14.*/`)
- **AND** fall back to `engine/3rdparty/python314/`
- **AND** accept both `.dylib` and framework layouts

#### Scenario: Python macOS linking
- **WHEN** the runtime is linked on macOS
- **THEN** it SHALL link against the macOS Python library (framework, dylib, or static lib)
- **AND** post-build commands SHALL copy the Python shared library and standard library to the output directory

### Requirement: GLFW builds for macOS with Cocoa backend

The GLFW third-party library SHALL build for macOS using the Cocoa backend. The existing `engine/3rdparty/glfw/premake5.lua` already lists macOS source files; they SHALL be enabled via `filter "system:macosx"`.

#### Scenario: GLFW macOS compilation
- **WHEN** GLFW is compiled on macOS
- **THEN** the Cocoa platform files (`cocoa_init.m`, `cocoa_window.m`, `cocoa_monitor.m`, etc.) SHALL be compiled
- **AND** `_GLFW_COCOA` SHALL be defined
- **AND** Win32 platform files SHALL NOT be compiled

### Requirement: GLAD builds for macOS

The GLAD OpenGL loader SHALL build on macOS without modification. No platform-specific changes are required for GLAD; the existing cross-platform C source SHALL be used.

#### Scenario: GLAD macOS compilation
- **WHEN** GLAD is compiled on macOS
- **THEN** it SHALL compile successfully with Apple Clang
- **AND** it SHALL link against the OpenGL framework at runtime (handled by GLFW context creation)

### Requirement: NFD (Native File Dialog Extended) is integrated

The Native File Dialog Extended library SHALL be added to `engine/3rdparty/nfd/` for cross-platform file dialogs. The engine SHALL link against it on all platforms.

#### Scenario: NFD is available in the build
- **WHEN** the editor project is built on any platform
- **THEN** NFD SHALL be compiled and linked
- **AND** the header `nfd.h` SHALL be includable
- **AND** `NFD_OpenDialog()` and `NFD_SaveDialog()` SHALL be callable

#### Scenario: NFD works on macOS
- **WHEN** `NFD_OpenDialog()` is called on macOS
- **THEN** it SHALL present a native Cocoa file open panel
- **AND** it SHALL NOT require any Objective-C code in the engine

### Requirement: PhysX is disabled on macOS

PhysX SHALL NOT be built or linked on macOS. The `PX_PHYSX_STATIC_LIB` define SHALL be excluded from macOS builds. All PhysX-dependent source code SHALL be gated behind `#ifdef PLATFORM_WINDOWS`.

#### Scenario: PhysX excluded from macOS build
- **WHEN** the engine is built on macOS
- **THEN** no PhysX headers SHALL be included in the build
- **AND** `PhysicsSystem`, `PhysicsComponent`, and `ColliderComponent` SHALL be excluded from compilation

### Requirement: Importer tool is Windows-only

The importer tool (`engine/tool/importer/`) and its GUI (`importer_gui.py`) SHALL remain Windows-only. No macOS adaptation is required for the importer in this change.

#### Scenario: Importer unchanged on macOS
- **WHEN** building on macOS
- **THEN** the importer project SHALL NOT be included in the build (or SHALL gracefully fail with a platform-not-supported message)

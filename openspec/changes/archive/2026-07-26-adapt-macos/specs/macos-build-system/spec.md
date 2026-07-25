## ADDED Requirements

### Requirement: Premake5 generates macOS build files

The build system SHALL support generating build files for macOS via Premake5. On macOS, `python dev/z1.py generate` SHALL invoke premake5 with the `gmake2` action to produce GNU Makefiles.

#### Scenario: Makefile generation on macOS
- **WHEN** a developer runs `python dev/z1.py generate` on macOS
- **THEN** premake5 SHALL be invoked with `gmake2` action
- **AND** a `Makefile` SHALL be generated at the repository root
- **AND** the command SHALL exit with code 0

#### Scenario: Windows generation unchanged
- **WHEN** a developer runs `python dev/z1.py generate` on Windows
- **THEN** the existing behavior SHALL be preserved (premake5 vs2022 --vs2026)

### Requirement: All premake5 projects support macOS platform filter

Every `premake5.lua` file in the project SHALL include a `filter "system:macosx"` block that specifies macOS-appropriate settings. This SHALL include platform defines, system frameworks, and architecture settings.

#### Scenario: macOS filter in root premake5.lua
- **WHEN** the root `premake5.lua` is processed on macOS
- **THEN** it SHALL define `PLATFORM_MACOS` for all projects
- **AND** it SHALL NOT set `toolset "v145"` (Visual Studio specific)

#### Scenario: macOS filter in runtime premake5.lua
- **WHEN** `engine/runtime/premake5.lua` is processed on macOS
- **THEN** it SHALL define `PLATFORM_MACOS`
- **AND** it SHALL link against `-framework OpenGL` instead of `opengl32.lib`
- **AND** it SHALL NOT use MSVC-specific flags (`/IGNORE:4006`, `/bigobj`)

#### Scenario: macOS filter in third-party premake files
- **WHEN** any `engine/3rdparty/*/premake5.lua` is processed on macOS
- **THEN** it SHALL include macOS-appropriate source files (e.g., GLFW cocoa_*.m)
- **AND** it SHALL use macOS system frameworks where needed (Cocoa, IOKit, CoreVideo)

### Requirement: PhysX is excluded from macOS builds

PhysX libraries and the `PX_PHYSX_STATIC_LIB` define SHALL be gated behind `filter "system:windows"` in the build system. On macOS, no PhysX libraries SHALL be linked, and `PX_PHYSX_STATIC_LIB` SHALL NOT be defined.

#### Scenario: PhysX not linked on macOS
- **WHEN** the runtime is linked on macOS
- **THEN** no PhysX libraries SHALL appear in the link step
- **AND** `PX_PHYSX_STATIC_LIB` SHALL NOT be defined

#### Scenario: PhysX still linked on Windows
- **WHEN** the runtime is linked on Windows
- **THEN** the existing PhysX linking behavior SHALL be preserved

### Requirement: Python 3.14 is linkable on macOS

The build system SHALL support linking against macOS Python 3.14, either as a system framework, a bundled `.dylib`, or a static library at `engine/3rdparty/python314/`.

#### Scenario: Python macOS linking
- **WHEN** the runtime is linked on macOS
- **THEN** it SHALL link against the macOS Python library (framework, dylib, or static lib)
- **AND** post-build commands SHALL copy the Python shared library and standard library to the output directory

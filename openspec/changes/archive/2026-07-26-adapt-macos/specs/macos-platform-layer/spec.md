## ADDED Requirements

### Requirement: PLATFORM_MACOS preprocessor define is available

When building on macOS, the `PLATFORM_MACOS` preprocessor define SHALL be set for all engine translation units. Code SHALL use `#ifdef PLATFORM_MACOS` to gate macOS-specific code paths.

#### Scenario: PLATFORM_MACOS is defined during macOS builds
- **WHEN** any `.cpp` or `.h` file is compiled on macOS
- **THEN** `PLATFORM_MACOS` SHALL be defined as a preprocessor macro
- **AND** `PLATFORM_WINDOWS` SHALL NOT be defined

#### Scenario: PLATFORM_MACOS is not defined during Windows builds
- **WHEN** any `.cpp` or `.h` file is compiled on Windows
- **THEN** `PLATFORM_MACOS` SHALL NOT be defined
- **AND** `PLATFORM_WINDOWS` SHALL be defined

### Requirement: core.h supports non-Windows platforms

The `API` macro in `engine/runtime/source/core/core.h` SHALL no longer emit a hard `#error` for non-Windows platforms. It SHALL define `API` as empty for macOS (no `__declspec` needed for static libraries).

#### Scenario: core.h compiles on macOS
- **WHEN** `core.h` is included during a macOS build
- **THEN** no `#error` SHALL be triggered
- **AND** `API` SHALL be defined as an empty macro

#### Scenario: core.h still works on Windows
- **WHEN** `core.h` is included during a Windows build
- **THEN** the existing DLL import/export behavior SHALL be preserved

### Requirement: Cross-platform file dialogs replace Win32 dialogs

The engine SHALL use a cross-platform native file dialog library instead of Win32 `OPENFILENAMEA` for open/save file dialogs. The Native File Dialog Extended (NFD) library SHALL be added to `engine/3rdparty/`.

#### Scenario: Open file dialog on macOS
- **WHEN** the editor invokes `open_file_dialog()` on macOS
- **THEN** a native macOS file open panel SHALL appear
- **AND** the selected file path SHALL be returned as a UTF-8 string
- **AND** no Win32 API calls SHALL be made

#### Scenario: Save file dialog on macOS
- **WHEN** the editor invokes `save_file_dialog()` on macOS
- **THEN** a native macOS file save panel SHALL appear
- **AND** the chosen path SHALL be returned as a UTF-8 string

#### Scenario: File dialogs still work on Windows
- **WHEN** the editor invokes `open_file_dialog()` or `save_file_dialog()` on Windows
- **THEN** native Windows file dialogs SHALL appear (via NFD or existing Win32 code)

### Requirement: POSIX terminal I/O replaces conio.h

The Python layer's console input (`_kbhit`/`_getch`) SHALL use POSIX `termios` on macOS instead of `<conio.h>`. On Windows, the existing `<conio.h>` code path SHALL be preserved.

#### Scenario: Non-blocking key check on macOS
- **WHEN** the embedded Python REPL needs to check for keyboard input on macOS
- **THEN** `_kbhit()` SHALL be implemented using `fcntl(F_GETFL)` and `read()` with `termios` in non-canonical mode
- **AND** it SHALL return immediately without blocking

#### Scenario: Key capture on macOS
- **WHEN** the embedded Python REPL reads a key press on macOS
- **THEN** `_getch()` SHALL be implemented using `termios` + `read()`
- **AND** it SHALL return the character read from stdin

#### Scenario: Console I/O still works on Windows
- **WHEN** the Python layer runs on Windows
- **THEN** the existing `<conio.h>` `_kbhit`/`_getch` code path SHALL be used

### Requirement: io.cpp Windows-reserved-name check is platform-gated

The Windows reserved device name check in `legalize_path()` SHALL only be active when `PLATFORM_WINDOWS` is defined. On macOS, the check SHALL be skipped.

#### Scenario: Path legalization on macOS
- **WHEN** `legalize_path()` is called on macOS with a path containing "CON" or "PRN"
- **THEN** the path SHALL be accepted as valid
- **AND** no error SHALL be produced

#### Scenario: Path legalization on Windows unchanged
- **WHEN** `legalize_path()` is called on Windows
- **THEN** the existing reserved-name rejection behavior SHALL be preserved

### Requirement: OpenGL 4.1 Core Profile context on macOS

On macOS, the GLFW window SHALL request an OpenGL 4.1 core profile context. OpenGL 4.2+ API calls (e.g., DSA functions) SHALL be gated behind `#ifdef PLATFORM_WINDOWS` with non-DSA fallback paths on macOS. On Windows, the existing OpenGL context creation behavior SHALL be preserved.

#### Scenario: GLFW window creation on macOS
- **WHEN** a GLFW window is created on macOS
- **THEN** `GLFW_OPENGL_PROFILE` SHALL be set to `GLFW_OPENGL_CORE_PROFILE`
- **AND** `GLFW_OPENGL_FORWARD_COMPAT` SHALL be set to `GLFW_TRUE`
- **AND** `GLFW_CONTEXT_VERSION_MAJOR` SHALL be 4
- **AND** `GLFW_CONTEXT_VERSION_MINOR` SHALL be 1

#### Scenario: GLFW window creation on Windows unchanged
- **WHEN** a GLFW window is created on Windows
- **THEN** the existing context creation behavior SHALL be preserved (no forward-compat requirement)

#### Scenario: DSA calls gated on Windows
- **WHEN** an OpenGL DSA function (e.g., `glCreateTextures`) is called
- **THEN** it SHALL be wrapped in `#ifdef PLATFORM_WINDOWS`
- **AND** an equivalent non-DSA fallback SHALL be provided for macOS (`glGenTextures` + `glBindTexture` + `glTexImage2D`)

### Requirement: PhysX is compiled out on macOS

All PhysX-dependent code (PhysicsSystem, PhysicsComponent, ColliderComponent) SHALL be gated behind `#ifdef PLATFORM_WINDOWS`. On macOS builds, these systems SHALL not be compiled or registered.

#### Scenario: No PhysX includes on macOS
- **WHEN** `scene.cpp` or `core.cpp` is compiled on macOS
- **THEN** no PhysX headers SHALL be included
- **AND** `PhysicsSystem` SHALL NOT be instantiated in `RuntimeContext`

#### Scenario: PhysX still functional on Windows
- **WHEN** the engine runs on Windows
- **THEN** the existing PhysX integration SHALL be fully functional

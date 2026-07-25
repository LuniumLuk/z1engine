## ADDED Requirements

### Requirement: Dev CLI detects host operating system

The `dev/z1.py` entry point SHALL detect the host operating system and expose it to all commands. Commands SHALL use this information to select platform-appropriate tooling.

#### Scenario: macOS detection
- **WHEN** `dev/z1.py` is run on macOS
- **THEN** `sys.platform` SHALL report `"darwin"`
- **AND** commands SHALL use macOS code paths (premake5, make, no .exe extensions)

#### Scenario: Windows detection
- **WHEN** `dev/z1.py` is run on Windows
- **THEN** `sys.platform` SHALL report `"win32"`
- **AND** the existing Windows behavior SHALL be preserved

### Requirement: generate command works on macOS

The `generate` command SHALL locate the Premake5 binary appropriate for the host OS and invoke it with the correct action.

#### Scenario: Premake5 binary discovery on macOS
- **WHEN** `generate` is run on macOS
- **THEN** it SHALL search for `premake5` on `PATH` first
- **AND** fall back to `utils/premake/premake5` (macOS binary)
- **AND** it SHALL NOT look for `premake5.exe`

#### Scenario: Premake5 invocation on macOS
- **WHEN** `generate` is run on macOS
- **THEN** it SHALL invoke `premake5 gmake2`
- **AND** it SHALL NOT pass `--vs2026`

### Requirement: compile command works on macOS

The `compile` command SHALL use the appropriate build tool for the host platform.

#### Scenario: Compilation with make on macOS
- **WHEN** `compile` is run on macOS
- **THEN** it SHALL invoke `make` with the generated Makefile
- **AND** it SHALL pass `config=<config>_x64` to select the build configuration
- **AND** it SHALL parse GCC/Clang error output for structured error reporting

#### Scenario: Windows compilation unchanged
- **WHEN** `compile` is run on Windows
- **THEN** the existing `devenv.com` behavior SHALL be preserved

### Requirement: test command discovers platform-appropriate executables

The `test` command SHALL search for test executables with the appropriate extension for the host OS.

#### Scenario: Test discovery on macOS
- **WHEN** `test` is run on macOS
- **THEN** it SHALL search for files matching `test_*` (no `.exe` extension) in the test output directory
- **AND** it SHALL NOT filter for `.exe` extension

#### Scenario: Test discovery on Windows
- **WHEN** `test` is run on Windows
- **THEN** the existing `test_*.exe` discovery SHALL be preserved

### Requirement: smoke command finds platform-appropriate executable

The `smoke` command SHALL search for the game or editor executable with the correct name for the host OS.

#### Scenario: Executable discovery on macOS
- **WHEN** `smoke` is run on macOS
- **THEN** it SHALL search for `game` and `editor` (no `.exe` extension) in the build output directory

#### Scenario: Executable discovery on Windows
- **WHEN** `smoke` is run on Windows
- **THEN** the existing `game.exe`/`editor.exe` search SHALL be preserved

### Requirement: release command packages macOS binaries

The `release` command SHALL package macOS-appropriate binaries and libraries.

#### Scenario: Release packaging on macOS
- **WHEN** `release` is run on macOS
- **THEN** it SHALL copy `editor` and `game` (no extension) instead of `.exe` files
- **AND** it SHALL copy `.dylib` files instead of `.dll` files
- **AND** it SHALL copy the Python framework or dylib

### Requirement: validate-shaders command finds macOS validator

The `validate-shaders` command SHALL find the shader validator binary with the correct name for the host OS.

#### Scenario: Validator discovery on macOS
- **WHEN** `validate-shaders` is run on macOS
- **THEN** it SHALL search for `shader_validator` (no extension) in the build output directory

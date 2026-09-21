# unity-build

## ADDED Requirements

### Requirement: Unity builds must be an explicit, opt-in generate mode

`python dev/z1.py generate --unity` on macOS MUST regenerate deterministic unity blob translation units and build the engine projects (runtime, editor, game, bakery) from them; plain `generate` MUST keep the per-TU file lists. Blobs MUST be written under `engine/intermediate/unity/<project>/`, be regenerated on every run, and MUST NOT be committed (the directory is gitignored).

#### Scenario: Unity mode generates blobs and blob-based project files
- **WHEN** `python dev/z1.py generate --unity` runs on macOS
- **THEN** every engine project MUST have blob files under `engine/intermediate/unity/<project>/`
- **AND** the generated Makefiles MUST compile the blobs instead of the individual sources for that project
- **AND** the command output MUST state that unity mode is active

#### Scenario: Default mode is unchanged
- **WHEN** `python dev/z1.py generate` runs (no `--unity`), or generation happens on Windows
- **THEN** the file lists and build flags MUST be identical to the pre-change behaviour (one translation unit per source, no blob references)

#### Scenario: Determinism
- **WHEN** `generate --unity` is run twice without source changes
- **THEN** the generated blob files MUST be byte-identical

### Requirement: Blob composition must keep unity-hostile sources standalone

Vendor implementation translation units (`source/3rdparty/*.cpp` and any `*_build.cpp`, e.g. stb/tinyexr/tinygltf/tinyobjloader/imgui/`opengl_imgui_build`) and `pch.cpp` MUST NOT be merged into blobs; they MUST keep compiling as individual translation units. All remaining sources of a project MUST appear in exactly one blob.

#### Scenario: Vendor TUs stay standalone
- **WHEN** the unity-blob file list for the runtime project is inspected
- **THEN** no blob may include a `3rdparty/*.cpp` or `*_build.cpp` source
- **AND** those sources MUST still appear as individual compile rules in the generated Makefile

#### Scenario: Every source is compiled exactly once
- **WHEN** a unity project is built from clean
- **THEN** each `.cpp` of the project MUST be pulled in by exactly one blob or standalone rule
- **AND** no symbol may be emitted twice because of overlapping blob membership

### Requirement: A unity build must be warning-free and produce a working binary

The unity build MUST meet the same quality bar as the default build on macOS: zero compiler warnings from engine sources, a successful link of the game target, and a running editor/game binary.

#### Scenario: Warning-free unity build
- **WHEN** the game target is built from clean with `--unity` on macOS
- **THEN** the build MUST report 0 errors and 0 warnings

#### Scenario: The binary built from blobs works
- **WHEN** `python dev/z1.py smoke --frames 10` runs against a unity-built `game` binary
- **THEN** the editor MUST start, render the frames and exit cleanly with 0 OpenGL errors

### Requirement: Unity mode must deliver a measured cold-build improvement

On the reference machine the runtime project's cold compile time MUST be at least 1.5x faster in unity mode than in the default per-TU mode (measured protocol: same flags, same `-j` setting, back-to-back runs), and the full-project numbers MUST be recorded in `openspec/kb/build.md`.

#### Scenario: Measured improvement is recorded
- **WHEN** the change is verified
- **THEN** the per-TU and unity cold-build wall clocks MUST both be recorded in `openspec/kb/build.md` together with the machine and protocol
- **AND** the unity runtime-project time MUST be at most two thirds of the per-TU time (verified measurement: 147 s → 70 s)

#### Scenario: Incremental cost is documented
- **WHEN** a developer reads the KB unity section
- **THEN** it MUST state that editing one source rebuilds its whole blob (measured ~50 s in the current grouping) and that the default per-TU build remains the choice for interactive development

### Requirement: Source-level unity blockers must be fixed, not excluded

Headers MUST carry include guards (`#pragma once`) so multi-inclusion inside a blob is safe, and third-party macro spellings MUST match what the vendor headers check (currently `GLFW_INCLUDE_NONE`). These fixes MUST apply to the default build as well.

#### Scenario: Guards exist
- **WHEN** engine headers are scanned after this change
- **THEN** every engine header MUST begin with `#pragma once`

#### Scenario: GLFW no longer pulls the legacy system GL header
- **WHEN** a translation unit that includes `GLFW/glfw3.h` is preprocessed after this change
- **THEN** Apple's `<OpenGL/gl.h>` MUST NOT be included transitively
- **AND** the project define MUST read `GLFW_INCLUDE_NONE`

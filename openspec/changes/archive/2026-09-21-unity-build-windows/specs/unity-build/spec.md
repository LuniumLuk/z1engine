# unity-build

## MODIFIED Requirements

### Requirement: Unity builds must be an explicit, opt-in generate mode

`python dev/z1.py generate --unity` on macOS and Windows MUST regenerate deterministic unity blob translation units and build the engine projects (runtime, editor, game, bakery) from them; plain `generate` MUST keep the per-TU file lists. Blobs MUST be written under `engine/intermediate/unity/<project>/`, be regenerated on every run, and MUST NOT be committed (the directory is gitignored). Both platforms MUST use the same generator and the same blob layout, with the platform filter expressed as `system:macosx or system:windows`.

#### Scenario: Unity mode generates blobs and blob-based project files
- **WHEN** `python dev/z1.py generate --unity` runs on macOS or Windows
- **THEN** every engine project MUST have blob files under `engine/intermediate/unity/<project>/`
- **AND** the generated Makefiles / `.vcxproj` files MUST compile the blobs instead of the individual sources for that project
- **AND** the command output MUST state that unity mode is active

#### Scenario: Default mode is unchanged
- **WHEN** `python dev/z1.py generate` runs (no `--unity`)
- **THEN** the file lists and build flags MUST be identical to the pre-change behaviour (one translation unit per source, no blob references)

#### Scenario: Determinism
- **WHEN** `generate --unity` is run twice without source changes
- **THEN** the generated blob files MUST be byte-identical

### Requirement: Blob composition must keep unity-hostile sources standalone

Vendor implementation translation units (`source/3rdparty/*.cpp` and any `*_build.cpp`, e.g. stb/tinyexr/tinygltf/tinyobjloader/imgui/`opengl_imgui_build`) and `pch.cpp` MUST NOT be merged into blobs; they MUST keep compiling as individual translation units. All remaining sources of a project MUST appear in exactly one blob. For a project that uses a precompiled header, every blob MUST include that header first, spelled exactly as the project's `/Yu` argument (MSVC requires it textually first and rejects path-qualified spellings and includes of sources that pull it in), and unity blobs MUST be compiled with `/bigobj` on Windows.

#### Scenario: Vendor TUs stay standalone
- **WHEN** the unity-blob file list for the runtime project is inspected
- **THEN** no blob may include a `3rdparty/*.cpp` or `*_build.cpp` source
- **AND** those sources MUST still appear as individual compile rules in the generated project

#### Scenario: Every source is compiled exactly once
- **WHEN** a unity project is built from clean
- **THEN** each `.cpp` of the project MUST be pulled in by exactly one blob or standalone rule
- **AND** no symbol may be emitted twice because of overlapping blob membership

#### Scenario: MSVC precompiled headers and section limits are handled
- **WHEN** the runtime blobs are compiled by MSVC
- **THEN** each blob MUST start with `#include "pch.h"` and MUST NOT fail with `C1010`
- **AND** a Debug unity build MUST NOT fail with `C1128` (unity builds on Windows compile with `/bigobj`)

### Requirement: A unity build must be warning-free and produce a working binary

The unity build MUST meet the same quality bar as the default build on macOS and Windows: zero compiler warnings from engine sources, a successful link of every target, and working binaries.

#### Scenario: Warning-free unity build
- **WHEN** the solution is built from clean with `--unity`
- **THEN** the build MUST report 0 failed projects, 0 errors and 0 warnings

#### Scenario: The binary built from blobs works
- **WHEN** `python dev/z1.py smoke --frames <N>` runs against a unity-built binary
- **THEN** the editor MUST start, render the frames and exit cleanly

#### Scenario: Test outcome matches the per-TU build
- **WHEN** `python dev/z1.py test --config <Cfg>` runs against a unity build for each configuration
- **THEN** the pass/fail set MUST be identical to the same configuration built per-TU

### Requirement: Unity mode must deliver a measured cold-build improvement

On each platform's reference machine, unity mode MUST be measurably faster than the default per-TU mode across all four configurations (Debug, Release, Profile, Hybrid), and the per-platform numbers MUST be recorded in `openspec/kb/build.md` together with the machine and protocol.

#### Scenario: Measured improvement is recorded
- **WHEN** the change is verified
- **THEN** per-TU and unity cold-build wall clocks for every configuration MUST be recorded in `openspec/kb/build.md` with the machine and protocol
- **AND** unity MUST be at least 1.5x faster than per-TU on the platform reference machines (macOS measurement: runtime project 147.1 s → 70.2 s; Windows measurement: 1.58x – 1.69x over all 23 projects)

#### Scenario: Incremental cost is documented
- **WHEN** a developer reads the KB unity section
- **THEN** it MUST state that editing one source rebuilds its whole blob and that the default per-TU build remains the choice for interactive development

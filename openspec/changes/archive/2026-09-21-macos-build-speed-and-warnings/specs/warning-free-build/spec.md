# Warning-free build

## ADDED Requirements

### Requirement: Engine code must compile without warnings under the default macOS clang warning set

A full clean Hybrid build on macOS MUST emit zero clang `warning:` diagnostics for engine sources (`engine/runtime`, `engine/editor`, `engine/game`, `engine/bakery`). The recorded baseline is 1619 warnings.

#### Scenario: Clean build is warning-free
- **WHEN** the Hybrid `game` target is rebuilt from clean and the build log is scanned for `warning:` lines from engine sources
- **THEN** the count MUST be zero
- **AND** the number MUST be recorded in the change's verification report and the KB

#### Scenario: Warnings stay visible
- **WHEN** a developer runs `./compile.sh` (macOS wrapper)
- **THEN** the wrapper MUST NOT pass `-w`, so new warnings are visible in the build output

### Requirement: Engine projects must compile clean under `-Wall -Wextra` on macOS

Engine projects MUST add `-Wall -Wextra` (macOS-scoped) on top of the default warning set, and MUST remain clean under it. The suppression list MUST be minimal and documented: only `-Wno-unused-parameter` is permitted project-wide (rewriting ~500 unnamed parameters carries no defect value); any additional suppression requires a written rationale in `premake5.lua` and the design document.

#### Scenario: Strict build is clean
- **WHEN** the engine projects are rebuilt from clean on macOS with the generated flags
- **THEN** the build log MUST NOT contain warnings from engine sources other than the documented suppression categories
- **AND** third-party projects MUST keep the default warning set (not `-Wall -Wextra`)

#### Scenario: Suppression list is explicit
- **WHEN** the macOS warning configuration in `premake5.lua` is inspected
- **THEN** every `-Wno-*` flag MUST appear with a comment explaining why the pattern is intentional

### Requirement: Defect-indicating warnings must be fixed, not suppressed

Warnings that describe undefined behaviour, memory-safety or format-string defects MUST be resolved in the code.

#### Scenario: Polymorphic deletion is safe
- **WHEN** events are destroyed through base-class pointers
- **THEN** every abstract polymorphic base used for polymorphic deletion (currently `z1::Event`) MUST have a virtual destructor
- **AND** the build MUST NOT emit `-Wdelete-abstract-non-virtual-dtor` or `-Wdelete-non-abstract-non-virtual-dtor` for engine classes

#### Scenario: No runtime format strings
- **WHEN** editor UI code displays runtime strings (asset names, uniform names, field names)
- **THEN** it MUST use `ImGui::TextUnformatted()` (or a literal format) instead of `ImGui::Text()` with a non-literal string
- **AND** the build MUST NOT emit `-Wformat-security`

#### Scenario: Format specifiers match argument types
- **WHEN** size/index values are printed in editor code
- **THEN** the format specifier MUST match the argument type (e.g. `%zu` for `size_t`/`size_type`, no `%llu`/`%d` mismatches)
- **AND** the build MUST NOT emit `-Wformat` for engine sources

#### Scenario: Enum switches handle every enumerator
- **WHEN** an RHI or asset conversion switches over an engine enum
- **THEN** it MUST either handle every enumerator or fall through to an explicit `default` branch with an assertion/log
- **AND** the build MUST NOT emit `-Wswitch` for engine sources

### Requirement: Reflection macros must contain their platform-specific warning scope

The `REFLECTED_FIELD` macro MUST keep `offsetof()` on non-standard-layout reflected types working on MSVC and clang without changing type layouts: the `FieldInfo` construction is wrapped in a clang-only diagnostic push/pop for `-Winvalid-offsetof`. This is the single permitted macro-level suppression; the rationale MUST be documented next to the macro. Aggregate-initialization warnings from the same macro MUST be fixed at the type level (`FieldInfo::widget` gets an explicit default initializer) rather than suppressed.

#### Scenario: No offsetof warnings
- **WHEN** any TU using `REFLECTED_FIELD` compiles on macOS
- **THEN** the build MUST NOT emit `-Winvalid-offsetof`
- **AND** the diagnostic pragmas MUST be guarded so MSVC never sees them

#### Scenario: Aggregate init warnings are fixed at the type level
- **WHEN** `FieldInfo` is inspected after this change
- **THEN** every member initialised implicitly by the reflection macros MUST have a default member initializer
- **AND** the strict build MUST NOT emit `-Wmissing-field-initializers` for reflection macros

### Requirement: Include hygiene warnings must be resolved without touching vendored code

Non-portable include paths MUST be corrected to match on-disk casing, engine headers MUST NOT pull vendored headers into unrelated translation units, and `engine/3rdparty/` MUST NOT be edited. The single known vendored-header warning on macOS (deprecated `sprintf` in `stb_image_write.h`) MUST be scoped to the two translation units that build stb (`engine/runtime/source/3rdparty/stb_build.cpp`, `engine/bakery/source/3rdparty/stb_build.cpp`) via a per-file suppression.

#### Scenario: Include paths match disk
- **WHEN** engine sources include GLFW/imgui headers
- **THEN** the include MUST use the on-disk casing
- **AND** the build MUST NOT emit `-Wnonportable-include-path`

#### Scenario: Editor header stops leaking vendored headers
- **WHEN** `editor_layer.h` is inspected after this change
- **THEN** it MUST NOT include `stb/stb_image_write.h` (the include belongs in the `.cpp` that needs it)

#### Scenario: Vendored warning is contained
- **WHEN** the build log is scanned for warnings originating in `engine/3rdparty/`
- **THEN** the only permitted occurrence MUST be the `stb` deprecation from the two `stb_build.cpp` translation units
- **AND** no file under `engine/3rdparty/` may be modified by this change

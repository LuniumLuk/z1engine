## ADDED Requirements

### Requirement: All code must follow z1engine formatting rules

Every source file modified or created by an agent MUST conform to the project's formatting conventions.

#### Scenario: Indentation
- **WHEN** code is written or modified
- **THEN** indentation MUST use hard tabs (not spaces)
- **AND** tab width is 4 columns for alignment purposes

#### Scenario: Brace style
- **WHEN** code contains control structures, function definitions, class/struct definitions
- **THEN** K&R brace style MUST be used (opening brace on same line)
- **AND** example: `if (cond) {`, `void foo() {`, `struct Bar {`

#### Scenario: Line endings
- **WHEN** a file is created or modified
- **THEN** line endings MUST be CRLF (Windows-style)

#### Scenario: Column limit
- **WHEN** code is written
- **THEN** lines SHOULD NOT exceed 120 columns

### Requirement: All code must follow z1engine naming conventions

#### Scenario: Type names
- **WHEN** a class, struct, or enum is declared
- **THEN** the name MUST use PascalCase (e.g., `RenderGraph`, `MeshComponent`, `ImageFormat`)

#### Scenario: Methods and functions
- **WHEN** a method or free function is declared
- **THEN** the name MUST use camelCase with underscores for word separation (e.g., `on_update`, `push_layer`, `create_frustum`)

#### Scenario: Member variables
- **WHEN** a non-static class/struct member variable is declared
- **THEN** the name MUST be prefixed with `m_` (e.g., `m_window`, `m_timer`, `m_should_exit`)

#### Scenario: File names
- **WHEN** a new source file is created
- **THEN** the name MUST be lowercase with underscores (e.g., `render_graph.cpp`, `vertex_array.h`)
- **AND** test files MUST follow the pattern `test_<name>.cpp`

#### Scenario: Namespace
- **WHEN** code is added to the engine
- **THEN** it MUST be within the `z1` namespace (or nested, e.g., `z1::bakery`)
- **AND** `using namespace z1;` is allowed only in `.cpp` files, never in headers

### Requirement: Const placement and type usage must follow project conventions

#### Scenario: Const references
- **WHEN** a const reference parameter or variable is declared
- **THEN** it MUST use east-const placement: `Type const&` (e.g., `std::string const& name`, `Filepath const& path`)

#### Scenario: Smart pointer passing
- **WHEN** a `std::shared_ptr<T>` is passed to a function
- **THEN** it MUST be passed by `std::shared_ptr<T> const&` (const reference to shared_ptr)

#### Scenario: Ownership model
- **WHEN** new ownership is introduced
- **THEN** shared ownership MUST use `std::shared_ptr`
- **AND** unique ownership MUST use `std::unique_ptr`
- **AND** raw pointers MUST only be used for non-owning references
- **AND** raw owning pointers are PROHIBITED

### Requirement: Include ordering must follow the project standard

#### Scenario: Include order in .cpp files
- **WHEN** a `.cpp` file has `#include` directives
- **THEN** the order MUST be:
  1. `#include "pch.h"` (first, mandatory in translation units that use PCH)
  2. Corresponding header (e.g., `foo.cpp` includes `"foo.h"`)
  3. Same-module engine headers (`"..."`)
  4. Other engine module headers (`"..."`)
  5. Third-party headers (`<...>`)
  6. Standard library headers (`<...>`)
- **AND** groups MUST be separated by a blank line

#### Scenario: Header guards
- **WHEN** a new `.h` file is created
- **THEN** it MUST use `#pragma once` (not `#ifndef` guards)

### Requirement: Prohibited patterns must never be introduced

#### Scenario: Prohibited code patterns
- **WHEN** code is written or modified
- **THEN** the following patterns MUST NOT be used:
  - `using namespace std;` (anywhere)
  - `std::endl` (use `"\n"` or `'\n'`)
  - `std::cerr` or `std::cout` for diagnostics (use `CORE_DEBUG`, `CORE_WARN`, `CORE_ERROR`)
  - Raw owning `new`/`delete` (use smart pointers)
  - `#ifndef`/`#define` header guards (use `#pragma once`)
  - Modifying files under `engine/3rdparty/` without explicit authorization

### Requirement: Logging and diagnostics must use engine utilities

#### Scenario: Log output
- **WHEN** diagnostic or status output is needed
- **THEN** the agent MUST use the engine logging macros:
  - `CORE_DEBUG(...)` -- debug info (stripped in Release)
  - `CORE_WARN(...)` -- warnings
  - `CORE_ERROR(...)` -- errors
  - `CLIENT_INFO(...)` -- client-facing info
- **AND** `PROFILE_FUNCTION()` MUST be added to any non-trivial function that could be a performance concern
- **AND** `PROFILE_SCOPE("name")` for specific blocks within a function

### Requirement: New files must integrate with the build system

#### Scenario: New source file created
- **WHEN** a new `.cpp` or `.h` file is added under `engine/`
- **THEN** it MUST follow the existing directory structure for its module
- **AND** the agent MUST verify the file is picked up by the premake5 glob patterns (check the relevant `premake5.lua`)
- **AND** if the file is not covered by an existing glob, the `premake5.lua` MUST be updated
- **AND** `dev\generate_vs2026.bat` MUST be re-run

#### Scenario: New test file created
- **WHEN** a new test file is added to `engine/test/`
- **THEN** it MUST follow the `test_<name>.cpp` naming pattern
- **AND** it will be auto-discovered by the `create_test()` function in root `premake5.lua`
- **AND** the agent MUST re-run `dev\generate_vs2026.bat` to pick it up

### Requirement: Changes must be minimal and surgical

#### Scenario: Scope of modifications
- **WHEN** an agent implements a task
- **THEN** modifications MUST be limited to files directly required by the task
- **AND** unrelated refactoring MUST NOT be performed
- **AND** if existing code near the change has style violations, the agent SHOULD fix them only in lines it's already modifying

#### Scenario: Preserving local conventions
- **WHEN** a file has local conventions that differ slightly from global rules
- **THEN** the agent MUST follow the local file's convention when modifying that file
- **AND** the agent SHOULD note the deviation in the KB update

### Requirement: Commits must follow project conventions

#### Scenario: Commit message format
- **WHEN** an agent creates a commit
- **THEN** the subject MUST use Conventional Commits format: `<type>: <description>`
- **AND** valid types are: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`
- **AND** AI-generated commits MUST include the `[AI]` prefix: `[AI] feat: add bloom pass`
- **AND** the trailer MUST include:
  ```
  Generated-by: <agent-name>
  Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
  ```

#### Scenario: AI attribution in files
- **WHEN** a file is fully generated or significantly rewritten by an agent
- **THEN** it MUST include a header: `// [AI-GENERATED] This file was generated by an AI agent.` (C++) or `<!-- [AI-GENERATED] -->` (Markdown)

### Requirement: Pre-commit checklist must be executed, not just read

#### Scenario: Before committing
- **WHEN** an agent is about to commit code
- **THEN** it MUST mechanically verify each item:
  - [ ] Hard tabs used for indentation (search for leading spaces in modified lines)
  - [ ] K&R brace style (no opening braces on their own line in modified code)
  - [ ] `pch.h` is the first include in new/modified `.cpp` files
  - [ ] Headers use `#pragma once`
  - [ ] Naming follows PascalCase (types), camelCase (functions), `m_` (members)
  - [ ] Const placement uses `Type const&`
  - [ ] No prohibited patterns introduced
  - [ ] No edits to `engine/3rdparty/` (unless authorized)
  - [ ] DCV loop passed (COMPILE at minimum)
  - [ ] KB update step completed
- **AND** the agent MUST report checklist results before committing
- **AND** any failed item MUST be fixed before the commit proceeds

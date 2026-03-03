AI Agent Coding Style Guidelines for z1engine

Purpose
- Capture observed coding conventions from the repository (excluding 3rdparty) and enforce constraints for future AI edits.
- Keep rules minimal and prescriptive so automated agents make consistent, low-risk changes.

Applies to
- All source under: editor/, runtime/, engine/, bakery/, test/, utils/ (do NOT edit 3rdparty/ except when explicitly updating a vendor).

General principles
- Make the smallest possible change to satisfy a request. Avoid broad refactors.
- Preserve existing idioms and patterns in the edited file; follow local file style if it differs slightly from the global rules.

Formatting
- Indentation: hard tabs for code indentation (use tabs, not spaces) to match existing files.
- Brace style: K&R (opening brace on same line as control/decl): e.g., "if (cond) {" and "Type::fn() {".
- Line endings: CRLF (Windows-style) for any edited files.
- File-level include ordering: precompiled header (pch.h) must be the first include in translation units that use it. After that, mimic the target file's existing order; when creating new files use groups separated by a blank line in this order: project headers ("\"...\""), third-party headers ("<...>") and standard library ("<...>") as appropriate.
- Header guards: use #pragma once in headers.

Naming conventions
- Namespace: code lives in namespace z1 (or nested, e.g., z1::bakery). Maintain that namespace for new code.
- Types (classes, structs, enums): PascalCase (e.g., Application, VertexArray, ShaderModule).
- Methods and functions: camelCase (e.g., on_event, push_layer, create).
- Member variables: prefix with m_ (e.g., m_window, m_timer, m_should_exit).
- File names: lowercase with underscores for C++ source and headers (e.g., shader.cpp, vertex_array.h). Tests follow test_*.cpp.
- Constants and macros: ALL_CAPS for macros, but repository has some lowercase constexpr (e.g., pi). Follow the context in the file being edited.

Type and const usage
- Prefer explicit smart pointers used in repository: std::shared_ptr is common; when passing, use const reference form: std::shared_ptr<T> const& param.
- Preferred const placement: use "Type const&" for const references (e.g., Filepath const& path, std::string const& src).
- Use noexcept on small functions where appropriate (observe existing style like free helper functions marked noexcept).

API and ABI
- Use provided macros and instrumentation: PROFILE_FUNCTION(), PROFILE_SCOPE(), CORE_DEBUG, BIND_EVENT_FN, and other repository-specific utilities when matching existing patterns.
- Where APIs use an "API" macro (e.g., struct API VertexArray), maintain that macro on exported types.

Includes and dependencies
- Do not modify files under 3rdparty/ unless performing an explicit vendor update and documenting it.
- When adding new third-party usage, prefer adding it via the existing 3rdparty/ folder and updating project/premake files only after confirmation from maintainers.

Memory and ownership
- Follow existing ownership model—shared ownership via std::shared_ptr is common. When introducing unique ownership, prefer std::unique_ptr but follow local patterns.
- Avoid introducing raw owning pointers; raw pointers may be used for non-owning references.

Logging, errors, and tests
- Use existing logging utilities (CORE_DEBUG, spdlog usage in core/log) for diagnostic messages.
- Add or update unit tests in test/ for any behavioral change; keep tests small and focused.

Comments and documentation
- Use brief // single-line comments for clarifications. Avoid large block comments unless necessary.
- Public API surfaces should have a short descriptive comment if adding new types or functions.

Style enforcement checklist for AI agents (must be satisfied before committing)
- [ ] Changes are minimal and localized to required files.
- [ ] Tabs used for indentation and K&R brace style maintained.
- [ ] pch.h included first in .cpp files that use it.
- [ ] Header files use #pragma once.
- [ ] Type, method and member naming follow PascalCase, camelCase, and m_ prefix rules.
- [ ] const placement uses "Type const&" and smart pointers passed by const&.
- [ ] No edits to 3rdparty/ unless explicitly authorized and documented.
- [ ] Include the Co-authored-by trailer in commit messages when the agent creates commits:

  Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>

Exceptions and local variance
- If a file clearly follows a different local convention (e.g., different include grouping), match that file's style when modifying it.
- When in doubt about ownership or high-impact API changes, create a draft PR and add a note for maintainers rather than committing directly.

Contact and escalation
- For design-level questions, open an ISSUE with a short summary and proposed change, tagging maintainers if known.

Document maintenance
- Update this document when conventions change (e.g., switching to spaces, adding a formatter, or modifying API rules).


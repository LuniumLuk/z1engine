AI Agent Setup for z1engine

Purpose
- Provide a concise onboarding and operational guide for future AI agents (and humans) working on this repository.

Repository snapshot
- Root files: premake scripts, Visual Studio solution: z1engine.sln
- Top-level directories:
  - 3rdparty/   : bundled libraries (glfw, yaml-cpp, stb, tinyexr, lz4, etc.)
  - asset/      : assets used by project
  - bakery/     : asset bakery / conversion tools
  - build/      : generated build artifacts
  - build-int/  : intermediate build artifacts
  - content/    : runtime content
  - editor/     : editor app
  - engine/     : core engine code
  - release/    : release packages
  - runtime/    : runtime/launcher
  - test/       : tests and test assets
  - utils/      : helper tools (contains premake binary under utils/premake)

Environment and platform
- Primary development platform: Windows (Windows_NT). Paths and tooling assume Windows-style paths (backslashes \).
- Visual Studio solution at the repo root: z1engine.sln
- Premake usage: scripts generate_vs2022.bat / generate_vs2026.bat and premake5.lua exist to generate project files.
- Build helpers: build_vs2022.bat, build_vs2026.bat are present for automated builds.

Building locally
- Typical steps (from repository root):
  1. Run `generate_vs2022.bat` or `generate_vs2026.bat` to regenerate solution/project files if modifying premake settings.
  2. Open z1engine.sln in Visual Studio (recommended) or run the provided build scripts: `build_vs2022.bat`.
  3. Use build/ and build-int/ for debugging intermediate outputs and artifacts.

Quick commands
- Generate projects: generate_vs2022.bat
- Build: build_vs2022.bat (or open z1engine.sln in Visual Studio)
- The utils/premake/premake5.exe binary is included for convenience; prefer using provided batches so environment variables are set correctly.

Third-party dependencies
- Many third-party libraries are vendored under 3rdparty/. Do not modify upstream source unless updating the dependency (and follow the repository's vendor update process).

Tests
- Tests are under test/. Use the existing test harnesses or scripts present in the repo to run tests locally. If no unified test runner is present, run individual test executables produced by the solution.

Development conventions and guidelines for AI agents
- Minimal, surgical changes: Make the smallest possible change to fix an issue or add a feature. Avoid broad refactors unless requested.
- Use Windows-style paths in all scripts and code modifications when interacting with the repository.
- Preserve third-party code: avoid editing files in 3rdparty/ except when intentionally updating a dependency. If updating, document the change and the upstream version.
- Commit messages: follow the existing style and ALWAYS include the required Co-authored-by trailer when creating commits on behalf of the agent:

  Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>

  (Add further author trailers as needed.)

- Branching and PRs:
  - Use descriptive branch names (kebab-case): e.g., feature/add-foo, fix/serialize-bar
  - Open a PR for any non-trivial change and include a concise description and testing notes.

- Tests and verification:
  - Run relevant tests and do a minimal build before opening a PR. If adding code, include or update tests where appropriate.
  - If running a full build is expensive, run at least the component-specific build and unit tests.

- Secrets and security:
  - Never commit secrets (API keys, private certs, credentials). If a secret is required for testing, use environment variables and document how to set them in local development notes (do not commit dotenv files with secrets).

Agent-specific operational rules
- Work in the repository root (D:\z1engine) unless a specific subdirectory is required.
- Use provided batch scripts for generating projects and building to ensure consistent environment setup.
- When making multiple edits, keep changes minimal and grouped in a single commit if they represent a single logical change.
- Run linters/builds/tests that are already part of the repo's tooling; do not add new global linters or CI steps without maintainer approval.

Reporting and intentions (for interactive AI tools)
- Before running repository-modifying operations, report the intent (brief, gerund form) and include the steps to be taken.
- Maintain a short changelog entry in the PR body summarizing the intent, files changed, and verification steps taken.

Contact and maintainers
- If unsure about design choices or high-impact changes, open a draft PR or contact the maintainers (add contact details here if available).

Appendix: Useful file pointers
- Solution: z1engine.sln
- Premake script: premake5.lua, utils\premake\premake5.exe
- Build scripts: build_vs2022.bat, build_vs2026.bat
- Generate scripts: generate_vs2022.bat, generate_vs2026.bat

90. Documentation Index
91. - ARCHITECTURE.md : High-level system overview.
92. - BUILDING.md     : Detailed build instructions.
93. - CONTRIBUTING.md : Guidelines for contributing code.
94. - ROADMAP.md      : Current development roadmap.

Notes
- This document is intentionally concise; update it as tooling and CI change.
- For questions about policies (branching, releases), refer to repository maintainers or add an ISSUE requesting clarification.

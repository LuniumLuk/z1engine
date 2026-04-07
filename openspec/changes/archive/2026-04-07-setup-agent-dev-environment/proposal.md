## Why

AI agents working on z1engine start every session cold. They must re-read 6 scattered documentation files (~400 lines total), infer the build workflow from batch scripts, guess at coding conventions from file examples, and have no structured way to update what they've learned. The result: wasted context tokens, inconsistent code quality, missed build steps, and knowledge that evaporates between sessions.

The current dev scripts (`dev/*.bat`) are dumb batch files with no exit codes, no structured output, no error parsing. Agents waste tokens reading noisy raw MSBuild output and guessing whether a build succeeded. The `cmd //c` shell-mangling workarounds documented in the standard-workflow spec are a symptom of the real problem: the tooling isn't agent-friendly.

The project already has an OpenSpec workflow for managing changes, but lacks the foundational infrastructure that makes agents consistently productive: a centralized knowledge base, an automated develop-compile-verify loop, enforceable code regulations, agent-friendly dev scripts, and documentation in a format optimized for agent consumption.

## What Changes

### 1. Create an Agent Knowledge Base (`openspec/kb/`)

Replace the current scatter of docs/ files with a single indexed, hierarchical, markdown-based knowledge base under `openspec/kb/`. The KB uses a numbered index file (`index.md`) pointing to topic files organized by domain. Each topic file is compact (no prose, no filler -- structured headings, tables, and code blocks only). Agents read the index first, then pull only the topics they need for the current task.

Structure:
```
openspec/kb/
  index.md                    -- master index with topic IDs and one-line descriptions
  architecture.md             -- system structure, modules, entry points, data flow
  build.md                    -- build system, commands, configurations, troubleshooting
  coding-style.md             -- all code conventions (formatting, naming, types, includes)
  contributing.md             -- workflow, commit format, branching, attribution
  project-map.md              -- directory tree, key files, file naming patterns
  shader-system.md            -- shader authoring, variant system, validation
  python-scripting.md         -- Python integration, pybind11, script lifecycle
  testing.md                  -- test infrastructure, how to add/run tests
  render-pipeline.md          -- render graph, forward/deferred, shared infrastructure
  asset-system.md             -- asset types, binary format, bakery, import pipeline
  ecs.md                      -- entity component system, scene, components, systems
```

### 2. Replace Batch Scripts with Python Dev-Scripts (`dev/`)

Rewrite all `dev/*.bat` scripts and root `.bat` launchers as Python CLI scripts under `dev/`. Each script provides:
- **Structured output**: clear `[OK]`, `[FAIL]`, `[SKIP]`, `[WARN]` prefixes on every status line
- **Normalized exit codes**: 0 = success, 1 = build error, 2 = validation error, 3 = runtime error
- **Machine-parseable summary**: final line is a JSON-like status the agent can parse without reading full output
- **Rich info**: error count, warning count, files changed, time elapsed
- **Input normalization**: accepts both forward/back slashes, relative/absolute paths, config names are case-insensitive

Scripts:
```
dev/
  z1.py                       -- unified CLI entry point: python dev/z1.py <command>
  commands/
    generate.py               -- premake5 project generation
    compile.py                -- MSBuild/devenv compilation with parsed output
    format.py                 -- code formatting (absorbs utils/format_code.py)
    validate_shaders.py       -- shader validation with per-shader results
    test.py                   -- test runner: discovers and runs test_*.exe
    smoke.py                  -- editor smoke test (--one-frame)
    dcv.py                    -- full DCV loop orchestrator (calls all above in sequence)
    release.py                -- package release folder
```

The old `.bat` files are kept as thin wrappers calling `python dev/z1.py <command>` for backward compatibility.

### 3. Setup a Standard Develop-Compile-Verify (DCV) Workflow

Extend the existing `standard-workflow` spec (from feat-particle) into a comprehensive DCV loop that agents execute via `python dev/z1.py dcv`. The loop enforces: generate (if needed) -> compile -> format -> validate shaders (if touched) -> run relevant tests -> smoke-test the editor. Each gate must pass before proceeding. Error recovery rules tell agents exactly what to do on failure. The DCV script outputs a single summary line agents can parse.

### 4. Establish Code Writing Regulations for Agents

Consolidate and strengthen the existing AI_AGENT_CODING_STYLE.md into a formal spec with enforceable WHEN/THEN rules. Add regulations currently missing: file creation rules, include ordering enforcement, prohibited patterns, error handling conventions, memory ownership rules, commit hygiene, and a mandatory pre-commit checklist that agents must execute (not just read).

### 5. Convert Existing Docs to Compact Agent-Optimized Format

Transform the 6 existing docs/ files into the KB topic files described in (1). The new format is:
- No prose paragraphs -- use headings, tables, bullet lists, code blocks
- Each fact stated once, in the most specific topic file
- Cross-references use `[topic-id]` links
- Every topic file starts with `> Summary:` one-liner and `> Scope:` scope

The original docs/ files are kept temporarily for human reference but marked deprecated via a header comment.

## Capabilities

### New Capabilities
- `agent-knowledge-base`: Indexed, hierarchical knowledge base that agents consult at session start and update after exploration
- `dev-scripts`: Python-based CLI dev tooling with structured output, normalized exit codes, and machine-parseable results
- `develop-compile-verify`: Automated build-test-verify loop with gates and error recovery, driven by `python dev/z1.py dcv`
- `agent-code-regulations`: Machine-enforceable coding rules with WHEN/THEN scenarios
- `compact-doc-format`: Token-efficient documentation format optimized for agent context windows

### Modified Capabilities
- `standard-workflow`: Extended with DCV automation, Python dev-scripts, test execution, and error recovery rules

## Impact

- **`dev/z1.py`**: New unified CLI entry point
- **`dev/commands/*.py`**: New Python dev-scripts (7 command modules)
- **`dev/*.bat`**: Replaced with thin wrappers calling Python CLI
- **`validate_shaders.bat`, `run_editor.bat`, `run_game.bat`**: Updated to call Python CLI
- **`utils/format_code.py`**: Absorbed into `dev/commands/format.py` (original kept as deprecated wrapper)
- **`openspec/kb/`**: New directory with ~12 topic files (created from existing docs content)
- **`openspec/specs/standard-workflow/spec.md`**: Major expansion with DCV loop, test gates, error recovery
- **`openspec/specs/dev-scripts/spec.md`**: New spec file
- **`openspec/specs/agent-code-regulations/spec.md`**: New spec file
- **`openspec/specs/agent-knowledge-base/spec.md`**: New spec file
- **`openspec/specs/compact-doc-format/spec.md`**: New spec file
- **`docs/*.md`**: Marked deprecated (not deleted), content migrated to KB
- **`openspec/config.yaml`**: Updated with project context pointing agents to KB
- **`.opencode/command/opsx-*.md`**: FORCE KB instruction injected into all 4 commands
- **`.github/prompts/opsx-*.prompt.md`**: FORCE KB instruction injected into all 4 prompts

## Deferred

- **Automated KB update tooling**: A CLI command that agents could run to append discoveries to KB files. For now, agents manually edit KB files following the format spec.
- **CI/CD integration**: Automated build/test pipeline in GitHub Actions. DCV loop is agent-driven for now.
- **KB versioning per branch**: Each branch could have branch-specific KB overlays. Deferred until multi-branch workflow patterns emerge.
- **Linting/static analysis integration**: clang-tidy or similar integrated into DCV. The project doesn't use these yet.
- **Interactive TUI**: Rich terminal UI with progress bars and color. Start with plain text structured output; add TUI later if needed.

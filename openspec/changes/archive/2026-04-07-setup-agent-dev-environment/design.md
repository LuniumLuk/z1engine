## Context

z1engine is a C++17 game engine (Windows/OpenGL/ImGui/EnTT) with an OpenSpec-driven AI agent workflow. Agents operate via four commands (`/opsx-propose`, `/opsx-apply`, `/opsx-archive`, `/opsx-explore`) defined in `.opencode/command/` and `.github/prompts/`. The project has 6 scattered docs under `docs/`, a `standard-workflow` spec on feat-particle, and coding style rules in `AI_AGENT_CODING_STYLE.md`. No centralized knowledge base exists. No automated build-verify loop is enforced. Agent knowledge evaporates between sessions. Dev tooling is 4 dumb batch scripts with no structured output, no exit codes, no error parsing -- agents waste tokens reading raw MSBuild noise and guessing outcomes.

## Goals

1. **Centralized knowledge base** -- All project knowledge in one indexed, hierarchical, markdown-based location (`openspec/kb/`) that agents read at session start and update at session end
2. **FORCE KB update on all subagents** -- Every agent command (propose, apply, archive, explore) must include a mandatory final step: check KB against discoveries made during the session, and update if stale or incomplete
3. **Python dev-scripts** -- Replace batch scripts with Python CLI tools that produce structured output, normalized exit codes, machine-parseable summaries, and handle all Windows shell quirks internally
4. **Automated DCV loop** -- Agents run a develop-compile-verify cycle after every code change via `python dev/z1.py dcv`, with gates and error recovery
5. **Enforceable code regulations** -- WHEN/THEN rules that agents check mechanically, not prose guidelines they might ignore
6. **Compact doc format** -- Token-efficient format spec for all KB files, replacing verbose human-oriented markdown

## Non-Goals

- CI/CD pipeline (DCV is agent-driven for now)
- Branch-specific KB overlays
- Replacing the OpenSpec workflow itself (this extends it)
- Interactive TUI / rich terminal colors (plain structured text first)

## Design Decisions

### D1: KB Structure -- Indexed Hierarchical Markdown

**Decision**: Use a flat directory `openspec/kb/` with an `index.md` master index and topic files.

**Alternatives considered**:
- Nested subdirectories (e.g., `kb/render/pipeline.md`, `kb/render/shaders.md`) -- Rejected: adds path depth without benefit for ~12 topics. Flat is simpler to index and navigate.
- Single monolithic file -- Rejected: too large for partial reads. Agents should load only relevant topics.
- YAML/JSON structured data -- Rejected: harder for agents to author naturally. Markdown with strict format rules achieves compactness while remaining writable.

**Format constraints**:
- `index.md`: numbered topic list with `[ID] topic-file.md -- one-line description`
- Each topic file: starts with `# <Title>`, then `> Summary: one-line`, then `> Scope: what files/dirs this covers`, then structured sections using headings, tables, bullet lists, code blocks only
- No prose paragraphs. No filler words. Every line carries information.
- Cross-references: `→ see [build.md]` or `→ see [coding-style.md#naming]`

### D2: FORCE KB Update -- Mandatory Agent Instruction

**Decision**: Inject a `## FORCE: Knowledge Base Protocol` section into every agent command file (`.opencode/command/opsx-*.md` and `.github/prompts/opsx-*.prompt.md`). This section is placed at the top of the command steps (after input parsing, before any other work) as a "read KB" step, AND at the bottom as an unconditional "update KB" step.

**Mechanism**:
```
FORCE INSTRUCTION -- APPLIES TO ALL AGENT COMMANDS:

SESSION START:
  1. Read openspec/kb/index.md
  2. Read topic files relevant to the current task
  3. Use KB content as authoritative project reference

SESSION END (MANDATORY -- execute even if main task failed):
  1. Review all files read, patterns discovered, errors encountered during session
  2. Compare against current KB topic files
  3. If any KB file is stale, incomplete, or missing information discovered this session:
     - Update the relevant topic file(s)
     - Update index.md if new topics were added
  4. Log what was updated (or "KB verified, no updates needed")
```

**Why FORCE**: Agents skip optional steps under context pressure. Making this unconditional and placing it in the command definition (not a guideline doc) ensures every subagent inherits it regardless of which command invoked the session.

**Alternatives considered**:
- Post-commit hook that reminds agents -- Rejected: easily skipped, only triggers on commits
- Separate `/opsx-update-kb` command -- Rejected: requires user to remember to invoke it
- Wrapper script around agent commands -- Rejected: no runtime exists to enforce this

### D3: Python Dev-Scripts -- Unified CLI with Structured Output

**Decision**: Rewrite all dev tooling as Python modules under `dev/commands/`, accessed via a single entry point `dev/z1.py`. No external dependencies -- stdlib only (subprocess, json, os, argparse, pathlib, time, glob).

**Why Python over batch**:
- Python is already in the project (embedded Python 3.14, plus `utils/*.py` scripts)
- Python handles Windows path normalization, subprocess management, and output parsing natively
- Agents can invoke `python dev/z1.py compile` without `cmd //c` wrappers or slash-mangling workarounds
- Structured JSON summary on the last line lets agents parse results by reading one line

**Architecture**:
```
dev/
  z1.py                       -- CLI router: parses command, dispatches to commands/<name>.py
  commands/
    __init__.py
    _common.py                -- shared utilities: run_subprocess, print_status, make_result
    generate.py               -- wraps premake5.exe
    compile.py                -- wraps devenv.com, parses MSBuild output
    format.py                 -- absorbs utils/format_code.py logic
    validate_shaders.py       -- wraps shader_validator.exe
    test.py                   -- discovers test_*.exe, runs them, aggregates results
    smoke.py                  -- wraps game.exe --one-frame
    dcv.py                    -- orchestrates generate->compile->format->shaders->test->smoke
    release.py                -- wraps create_release logic
```

**Output contract** (all commands):
```
[RUN] compile --config Debug
[INFO] Invoking devenv.com z1engine.sln /Build "Debug|x64"
[WARN] engine/runtime/source/render/shader.cpp(42): warning C4100: unreferenced parameter
[FAIL] engine/runtime/source/scene/scene.cpp(120): error C2065: undeclared identifier
[OK] compile completed (1 error, 1 warning, 12.3s)
RESULT: {"status": "fail", "command": "compile", "errors": 1, "warnings": 1, "elapsed": "12.3s"}
```

Exit codes: 0=ok, 1=build error, 2=validation error, 3=runtime error, 4=config error.

**Alternatives considered**:
- PowerShell scripts -- Rejected: less portable, agents still need escape workarounds
- Keep batch + add a Python parser -- Rejected: half-measures, still two layers of indirection
- Makefile / CMake -- Rejected: project uses premake5, adding another build system creates confusion

### D4: DCV Loop -- Driven by `python dev/z1.py dcv`

**Decision**: The DCV loop is a single command `python dev/z1.py dcv` that orchestrates all verification steps. Agents call this one command instead of invoking 6 scripts manually.

**DCV loop steps** (all via Python now):
```
1. GENERATE  -- python dev/z1.py generate  (only if premake5.lua or file list changed)
2. COMPILE   -- python dev/z1.py compile   (must exit 0, errors=0)
3. FORMAT    -- python dev/z1.py format
4. SHADERS   -- python dev/z1.py validate-shaders  (only if .glsl modified)
5. TESTS     -- python dev/z1.py test       (only if test-adjacent code changed)
6. SMOKE     -- python dev/z1.py smoke      (only after full task group)
```

**Auto-detection mode**: `python dev/z1.py dcv --auto` uses `git diff --name-only` to determine which steps to enable based on changed file types.

Each step has:
- **Trigger condition**: when to run it
- **Success criteria**: `RESULT` JSON with `"status": "ok"`
- **Failure action**: agent fixes code and re-runs DCV
- **Skip condition**: when it's safe to skip (reported as `[SKIP]` in output)

### D5: Code Regulations -- Spec with Enforceable Scenarios

**Decision**: Create `agent-code-regulations/spec.md` with WHEN/THEN scenarios extracted from AI_AGENT_CODING_STYLE.md plus new rules. Unlike the prose style guide, each rule is a testable condition.

**New rules not in current style guide**:
- File creation: MUST match existing naming patterns, MUST add to premake5.lua
- Prohibited patterns: no `using namespace std`, no raw owning pointers, no `std::endl`
- Error handling: use CORE_DEBUG/CORE_WARN/CORE_ERROR, never `std::cerr`
- Include ordering: pch.h -> same-module headers -> other engine headers -> 3rdparty -> std
- Pre-commit checklist: agent MUST execute (verify tabs, verify naming, verify includes) not just read

### D6: Compact Doc Format -- Spec for KB File Authoring

**Decision**: Define a format specification that all KB files must follow. This ensures consistency and token efficiency.

**Format rules**:
| Element | Rule |
|---------|------|
| Headings | `#` = topic title, `##` = section, `###` = subsection. No deeper. |
| Summary | `> Summary:` blockquote immediately after title. One line. |
| Scope | `> Scope:` blockquote after summary. Lists dirs/files covered. |
| Body | Tables, bullet lists, code blocks only. No paragraphs. |
| Cross-ref | `→ see [filename.md]` or `→ see [filename.md#section]` |
| Code | Fenced blocks with language tag. Inline backticks for identifiers. |
| Line budget | Each topic file SHOULD be under 150 lines. Split if larger. |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| KB becomes stale if FORCE instruction is ignored by a non-OpenSpec agent | KB is also human-readable; periodic manual review can catch drift |
| DCV loop adds time to every task | Skip conditions and `--auto` mode reduce unnecessary steps |
| Code regulations are too rigid for edge cases | "Exceptions" section in spec allows documented deviations |
| Duplicate content between KB and old docs/ during transition | Deprecation headers in old docs direct readers to KB; remove docs/ in a follow-up change |
| FORCE instruction increases token usage per session | KB index + 1-2 topic files is ~200 lines. Cheaper than re-discovering project structure each time |
| Python scripts add a Python dependency for dev workflow | Python 3.14 is already bundled with the project; system Python also works |
| Batch wrappers may confuse developers who expect old behavior | Wrappers are transparent -- same invocation, better output |

## File Organization (After Change)

```
dev/
  z1.py                                -- NEW: unified CLI entry point
  commands/                            -- NEW: Python command modules
    __init__.py
    _common.py                         -- shared utilities
    generate.py
    compile.py
    format.py
    validate_shaders.py
    test.py
    smoke.py
    dcv.py
    release.py
  build_vs2026.bat                     -- MODIFIED: thin wrapper -> python dev/z1.py compile
  compile_vs2026.bat                   -- MODIFIED: thin wrapper -> python dev/z1.py compile
  generate_vs2026.bat                  -- MODIFIED: thin wrapper -> python dev/z1.py generate
  create_release.bat                   -- MODIFIED: thin wrapper -> python dev/z1.py release

openspec/
  config.yaml                          -- updated with KB pointer
  kb/                                  -- NEW: agent knowledge base
    index.md                           -- master index
    architecture.md                    -- system structure
    build.md                           -- build commands and config
    coding-style.md                    -- code conventions
    contributing.md                    -- workflow and attribution
    project-map.md                     -- directory tree and key files
    shader-system.md                   -- shader authoring and validation
    python-scripting.md                -- Python integration
    testing.md                         -- test infrastructure
    render-pipeline.md                 -- rendering system
    asset-system.md                    -- asset types and pipeline
    ecs.md                             -- ECS, scenes, components
  specs/
    standard-workflow/spec.md          -- MODIFIED: extended with DCV loop via Python scripts
    dev-scripts/spec.md                -- NEW: Python dev tooling spec
    agent-knowledge-base/spec.md       -- NEW: KB structure and update rules
    agent-code-regulations/spec.md     -- NEW: enforceable coding rules
    compact-doc-format/spec.md         -- NEW: format spec for KB files
  changes/
    setup-agent-dev-environment/       -- this change

.opencode/command/
  opsx-propose.md                      -- MODIFIED: FORCE KB instruction added
  opsx-apply.md                        -- MODIFIED: FORCE KB instruction added
  opsx-archive.md                      -- MODIFIED: FORCE KB instruction added
  opsx-explore.md                      -- MODIFIED: FORCE KB instruction added

.github/prompts/
  opsx-propose.prompt.md               -- MODIFIED: FORCE KB instruction added
  opsx-apply.prompt.md                 -- MODIFIED: FORCE KB instruction added
  opsx-archive.prompt.md               -- MODIFIED: FORCE KB instruction added
  opsx-explore.prompt.md               -- MODIFIED: FORCE KB instruction added

validate_shaders.bat                   -- MODIFIED: thin wrapper -> python dev/z1.py validate-shaders
run_editor.bat                         -- kept as-is (not a dev tool)
run_game.bat                           -- kept as-is (not a dev tool)

docs/
  *.md                                 -- MODIFIED: deprecated headers added
```

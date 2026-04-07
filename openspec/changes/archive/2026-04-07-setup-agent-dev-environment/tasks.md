## 1. Python Dev-Scripts Infrastructure

- [x] 1.1 Create `dev/z1.py` -- CLI entry point: argparse router that dispatches to `dev/commands/<name>.py`. Print usage listing all 8 commands when invoked with no args.
- [x] 1.2 Create `dev/commands/__init__.py` (empty).
- [x] 1.3 Create `dev/commands/_common.py` -- shared utilities: `run_subprocess(cmd, cwd)`, `print_status(prefix, msg)` with `[OK]`/`[FAIL]`/`[WARN]`/`[SKIP]`/`[INFO]`/`[RUN]` prefixes, `make_result(status, command, **fields)` that outputs `RESULT: {json}` line, `normalize_config(name)` (case-insensitive -> `Debug`/`Release`/`Profile`), `find_vs2026()` (probe Community/Professional/Enterprise), `repo_root()`.
- [x] 1.4 Create `dev/commands/generate.py` -- wraps `utils/premake/premake5.exe vs2022 --vs2026`, cwd=repo root, exit code 0 or 4.
- [x] 1.5 Create `dev/commands/compile.py` -- wraps `devenv.com z1engine.sln /Build "<Config>|x64"`, parses MSBuild output for error/warning lines, reports `[FAIL]`/`[WARN]` per line, summary with error+warning counts. Exit codes: 0 ok, 1 build error, 4 VS not found.
- [x] 1.6 Create `dev/commands/format.py` -- absorb logic from `utils/format_code.py` (tabs, trailing whitespace, CRLF, ASCII check). Support `--dry-run`. Report `[INFO] Fixed: <path>` and `[WARN] Non-ASCII: <path>:<line>`. Summary with `"files_fixed": N`.
- [x] 1.7 Create `dev/commands/validate_shaders.py` -- wraps `engine/bin/Debug/shader_validator.exe`, parses per-shader pass/fail, exit code 0 or 2.
- [x] 1.8 Create `dev/commands/test.py` -- discovers `engine/bin/test/Debug/test_*.exe`, runs each as subprocess, reports `[OK]`/`[FAIL]` per test, summary with `"passed"/"failed"/"total"`. Support `--filter <glob>`. Exit code 0 or 3.
- [x] 1.9 Create `dev/commands/smoke.py` -- runs `engine/bin/Debug/game.exe --one-frame=<N>`, captures stdout/stderr, reports ok or crash. Exit code 0 or 3.
- [x] 1.10 Create `dev/commands/dcv.py` -- orchestrates generate->compile->format->validate-shaders->test->smoke in order. Support `--auto` (git diff file detection), `--generate`, `--shaders`, `--test`, `--smoke` flags. Stop on first failure. Output per-step `[OK]`/`[SKIP]`/`[FAIL]` lines plus aggregate `RESULT` JSON.
- [x] 1.11 Create `dev/commands/release.py` -- wraps existing `create_release.bat` logic (copy binaries+content to release folder).
- [x] 1.12 Verify: run `python dev/z1.py` (no args) and confirm it prints the command listing. Run `python dev/z1.py compile --help` and confirm it prints usage.

## 2. Convert Old Batch Scripts to Thin Wrappers

- [x] 2.1 Rewrite `dev/generate_vs2026.bat` to: `@echo off` / `python "%~dp0z1.py" generate %*` / `exit /b %ERRORLEVEL%`
- [x] 2.2 Rewrite `dev/compile_vs2026.bat` to call `python "%~dp0z1.py" compile %*`.
- [x] 2.3 Rewrite `dev/build_vs2026.bat` to call `python "%~dp0z1.py" compile %*` (build and compile are the same now).
- [x] 2.4 Rewrite `dev/create_release.bat` to call `python "%~dp0z1.py" release %*`.
- [x] 2.5 Rewrite `validate_shaders.bat` (repo root) to call `python dev/z1.py validate-shaders %*`.

## 3. Knowledge Base Creation

- [x] 3.1 Create `openspec/kb/index.md` -- master index with `[NN] filename.md -- description` entries for all ~12 topic files.
- [x] 3.2 Create `openspec/kb/architecture.md` -- migrated from `docs/ARCHITECTURE.md`, compact format.
- [x] 3.3 Create `openspec/kb/build.md` -- migrated from `docs/BUILDING.md`, includes Python dev-scripts commands.
- [x] 3.4 Create `openspec/kb/coding-style.md` -- migrated from `docs/AI_AGENT_CODING_STYLE.md`, enforceable WHEN/THEN format.
- [x] 3.5 Create `openspec/kb/contributing.md` -- migrated from `docs/CONTRIBUTING.md` and `docs/NEW_TASK_TEMPLATE.md`.
- [x] 3.6 Create `openspec/kb/project-map.md` -- directory tree, key files, file naming patterns (new, derived from codebase exploration).
- [x] 3.7 Create `openspec/kb/shader-system.md` -- shader authoring, variant system, validation (extracted from architecture + codebase).
- [x] 3.8 Create `openspec/kb/python-scripting.md` -- migrated from `docs/PLAN_PYTHON_SCRIPTING.md`, Python integration details.
- [x] 3.9 Create `openspec/kb/testing.md` -- test infrastructure, how to add/run tests (new, derived from codebase + test spec).
- [x] 3.10 Create `openspec/kb/render-pipeline.md` -- rendering system docs (extracted from architecture + archived specs).
- [x] 3.11 Create `openspec/kb/asset-system.md` -- asset types, binary format, bakery, import pipeline (extracted from codebase).
- [x] 3.12 Create `openspec/kb/ecs.md` -- ECS, scenes, components, systems (extracted from codebase).
- [x] 3.13 Verify: confirm every file listed in `index.md` exists, and every `.md` file in `openspec/kb/` (except `index.md`) is in the index.

## 4. FORCE KB Instruction Injection

- [x] 4.1 Add `## FORCE: Knowledge Base Protocol` section to `.opencode/command/opsx-propose.md` (before command-specific steps, with session-start read + session-end unconditional update).
- [x] 4.2 Add `## FORCE: Knowledge Base Protocol` section to `.opencode/command/opsx-apply.md`.
- [x] 4.3 Add `## FORCE: Knowledge Base Protocol` section to `.opencode/command/opsx-archive.md`.
- [x] 4.4 Add `## FORCE: Knowledge Base Protocol` section to `.opencode/command/opsx-explore.md`.
- [x] 4.5 Add `## FORCE: Knowledge Base Protocol` section to `.github/prompts/opsx-propose.prompt.md`.
- [x] 4.6 Add `## FORCE: Knowledge Base Protocol` section to `.github/prompts/opsx-apply.prompt.md`.
- [x] 4.7 Add `## FORCE: Knowledge Base Protocol` section to `.github/prompts/opsx-archive.prompt.md`.
- [x] 4.8 Add `## FORCE: Knowledge Base Protocol` section to `.github/prompts/opsx-explore.prompt.md`.

## 5. Sync Specs to `openspec/specs/`

- [x] 5.1 Copy `specs/dev-scripts/spec.md` to `openspec/specs/dev-scripts/spec.md`.
- [x] 5.2 Copy `specs/agent-knowledge-base/spec.md` to `openspec/specs/agent-knowledge-base/spec.md`.
- [x] 5.3 Copy `specs/agent-code-regulations/spec.md` to `openspec/specs/agent-code-regulations/spec.md`.
- [x] 5.4 Copy `specs/compact-doc-format/spec.md` to `openspec/specs/compact-doc-format/spec.md`.
- [x] 5.5 Create `openspec/specs/standard-workflow/spec.md` -- extend the feat-particle version with DCV loop automation via Python dev-scripts, referencing `dev-scripts/spec.md` and `develop-compile-verify/spec.md`.

## 6. Deprecate Old Docs

- [x] 6.1 Add deprecation header to `docs/ARCHITECTURE.md`: `> **DEPRECATED** — This document has been migrated to openspec/kb/architecture.md. This file is kept for reference only.`
- [x] 6.2 Add deprecation header to `docs/BUILDING.md`.
- [x] 6.3 Add deprecation header to `docs/AI_AGENT_CODING_STYLE.md`.
- [x] 6.4 Add deprecation header to `docs/CONTRIBUTING.md`.
- [x] 6.5 Add deprecation header to `docs/NEW_TASK_TEMPLATE.md`.
- [x] 6.6 Add deprecation header to `docs/PLAN_PYTHON_SCRIPTING.md`.

## 7. Update OpenSpec Config

- [x] 7.1 Update `openspec/config.yaml` to add `kb_path: openspec/kb` and `dev_scripts: dev/z1.py` entries, plus any project context fields agents need.

## 8. Verification

- [x] 8.1 Run `python dev/z1.py generate` and confirm `RESULT` line with `"status": "ok"`.
- [x] 8.2 Run `python dev/z1.py compile` and confirm the build succeeds (or parse pre-existing errors correctly).
- [x] 8.3 Run `python dev/z1.py format --dry-run` and confirm structured output.
- [x] 8.4 Run `python dev/z1.py test` and confirm test discovery and execution.
- [x] 8.5 Run `python dev/z1.py dcv --auto` end-to-end and confirm aggregate RESULT output.
- [x] 8.6 Verify all 8 agent command/prompt files contain the `## FORCE: Knowledge Base Protocol` section.
- [x] 8.7 Verify KB `index.md` is consistent with on-disk files (no orphaned entries, no missing files).

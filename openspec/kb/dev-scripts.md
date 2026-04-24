# Dev Scripts
> Summary: Python CLI reference for z1engine development tools (dev/z1.py)
> Scope: dev/z1.py, dev/commands/

## Entry Point

```cmd
python dev/z1.py <command> [options]
```

## Commands

| Command | Module | Description |
|---------|--------|-------------|
| `generate` | `commands/generate.py` | Regenerate VS project files via premake5 |
| `compile` | `commands/compile.py` | Build solution with MSBuild output parsing |
| `format` | `commands/format_cmd.py` | Code formatting (tabs, whitespace, CRLF) |
| `validate-shaders` | `commands/validate_shaders.py` | GLSL shader validation |
| `test` | `commands/test.py` | Discover and run test executables |
| `benchmark` | `commands/benchmark.py` | Run benchmark suites and baseline regression checks |
| `smoke` | `commands/smoke.py` | Editor startup smoke test |
| `dcv` | `commands/dcv.py` | Full develop-compile-verify loop |
| `release` | `commands/release.py` | Package release folder |

## Output Format

- Status lines prefixed with: `[OK]`, `[FAIL]`, `[WARN]`, `[SKIP]`, `[INFO]`, `[RUN]`
- Final line: `RESULT: {"status": "ok|fail|warn", "command": "<name>", ...}`
- Agents parse the RESULT line for pass/fail decisions

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Build/compilation error |
| 2 | Validation error (shaders, format) |
| 3 | Runtime error (test failure, crash) |
| 4 | Configuration error (missing tools) |

## Common Options

- `--config <Debug|Release|Profile>` -- build configuration (case-insensitive)
- `--help` -- command-specific usage
- `--dry-run` -- (format only) report without modifying

## DCV Loop

```cmd
python dev/z1.py dcv --auto        # auto-detect steps from git diff
python dev/z1.py dcv --all         # run all steps
python dev/z1.py dcv --test --smoke  # force specific steps
python dev/z1.py benchmark --suite runtime-core --config Debug
```

### Step Order

1. `generate` -- if `.lua` files changed
2. `compile` -- always
3. `format` -- always
4. `validate-shaders` -- if `.glsl` files changed
5. `test` -- if engine code changed
6. `benchmark` -- if mapped by changed paths or explicitly requested
7. `smoke` -- if requested

## Validation Report

- `dcv` writes structured reports to:
	- `dev/validation/reports/latest.json`
	- `dev/validation/reports/dcv-<timestamp>.json`
- Report includes:
	- aggregate verdict (`status`, `correctness`, `performance`)
	- selected test and benchmark suites
	- per-step payloads and environment metadata

## Shared Utilities (`commands/_common.py`)

- `repo_root()` -- resolve repository root path
- `find_vs2026()` -- detect VS2026 installation
- `normalize_config(name)` -- case-insensitive config normalization
- `run_subprocess(cmd, cwd, timeout)` -- run external command
- `make_result(status, command, **fields)` -- print RESULT JSON line
- `Timer` -- elapsed time measurement

-> see [build.md]

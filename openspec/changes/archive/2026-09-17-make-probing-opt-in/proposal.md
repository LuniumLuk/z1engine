## Why

`ENABLE_PROBING` is currently defined unconditionally for the `Hybrid` configuration, so every Hybrid build
carries the prober (per-frame timing calls, GL timer queries, periodic `[probe]` logging). Probing is a
diagnostic tool, not a shipping default — most Hybrid/F5 sessions want the plain optimized-with-asserts
build, and the periodic report is noise for them.

## What Changes

- Probing becomes **opt-in at project-generation time**: a new premake5 option `--probing` gates the
  `ENABLE_PROBING` define (still only for the `Hybrid` configuration). Without the option, no configuration
  defines it.
- `dev/z1.py generate` (and therefore `./generate.sh`, the VS `.bat` wrappers) accepts `--probing` and
  forwards it to premake5; `generate` also reports the probing state it just generated.
- Docs and the knowledge base are updated to the new invocation; the probing spec's compile-time gating
  requirement is modified accordingly.

## Capabilities

### Modified Capabilities
- `performance-probing`: the compile-time gating requirement now states that `ENABLE_PROBING` is defined
  only when generation ran with `--probing` and the `Hybrid` configuration is built.

## Impact

- `premake5.lua` (new option + filter), `dev/commands/generate.py` (`--probing` passthrough + status line)
- `openspec/kb/perf-probing-and-quality.md`, `openspec/kb/dev-scripts.md`, `docs/PROFILING_MACOS_OPENGL.md`
- BREAKING (build workflow): a Hybrid build only contains the prober after `generate --probing`; scripts or
  habits that relied on "Hybrid = probing" must pass the flag. Regenerating once (without it) makes Hybrid
  builds probe-free.

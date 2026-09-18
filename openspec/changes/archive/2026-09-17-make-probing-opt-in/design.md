## Context

`ENABLE_PROBING` is a compile-time gate: premake5 bakes the define into the generated Makefiles /
`.vcxproj` files, so "enabling probing" is a **generate-time** decision followed by a normal Hybrid build.
The flag was initially hard-wired to the `Hybrid` configuration, which made every Hybrid build pay for the
prober whether or not it was wanted.

## Goals / Non-Goals

- Goal: default generation produces probe-free builds in every configuration (no timing code, no GL query
  ring, no `[probe]` logging, no overhead).
- Goal: one obvious switch — `./generate.sh --probing` (macOS) or `python dev/z1.py generate --probing`
  (any platform), then build `Hybrid` as usual.
- Non-goal: runtime toggling — the gate stays compile-time (that is what keeps disabled builds free).
- Non-goal: changing which configuration can host probing (still `Hybrid`) or changing the env knobs.

## Decisions

### D1. premake5 option `--probing` gating the define

```lua
newoption { trigger = "probing", description = "..." }

filter { "configurations:Hybrid", "options:probing" }
	defines { "ENABLE_PROBING" }
```

Alternatives rejected: a `Probing` configuration (a fifth configuration to maintain, and it would lose the
Hybrid `DEBUG`+`ENABLE_ASSERTS` combination developers actually use); an env var read by premake5 (not
reproducible, hides state from the generated files); runtime switching (defeats the zero-cost gate).

### D2. `--probing` travels through the dev CLI

`dev/commands/generate.py` gains `--probing` and appends it to the premake5 command line for both the
`gmake` (macOS) and `vs2022 --vs2026` (Windows) paths, so `./generate.sh --probing`, `generate_vs2026.bat`
and `python dev/z1.py generate --probing` all behave identically. `dcv` keeps generating **without** the
flag: its compile/verify steps describe the plain build, and a probing verification run is an explicit
`generate --probing` + `compile` + run.

### D2a. Flipping the flag removes stale Hybrid objects

Object files do not depend on the generated projects (the gmake rules are
`$(OBJDIR)/x.o: source/x.cpp`), so regenerating after a flag change left probe-enabled objects in place and
the next build was a silent no-op — the "cheap" switch would have lied. `generate` therefore compares the
state baked into the current project files with the requested one and, when they differ, deletes
`engine/intermediate/Hybrid` and warns that the next Hybrid build recompiles everything. The deletion only
happens on an actual flip (no stamp file needed — the generated file is the state).

### D3. State is reported where it is decided

`generate` prints whether probing was enabled for the run, because a stale Makefile/`.vcxproj` (generated
earlier with a different flag) is the one realistic failure mode — you must regenerate after changing the
flag, and `compile` cannot detect it.

## Risks / Trade-offs

- Forgetting to regenerate after `--probing` means the flag appears to do nothing → mitigated by the
  printed status line and the KB instructions ("regenerate after toggling").
- Tooling that assumed `Hybrid` implies probing must now pass the flag (none in-repo: `dcv`, smoke and
  benchmark commands never read `ENABLE_PROBING`).

## Migration Plan

- To keep probing: `./generate.sh --probing && ./compile.sh --config Hybrid` (or
  `python dev/z1.py generate --probing` + `compile`).
- Otherwise regenerate once; Hybrid builds become probe-free and slightly faster.

## Open Questions

None.

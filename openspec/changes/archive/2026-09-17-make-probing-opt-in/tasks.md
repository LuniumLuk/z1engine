## 1. Build-flag plumbing

- [x] 1.1 Add the `probing` premake5 option and gate the Hybrid `ENABLE_PROBING` define on it
      (`filter { "configurations:Hybrid", "options:probing" }`)
- [x] 1.2 Add `--probing` to `dev/z1.py generate`, forward it to premake5 on both the `gmake` and
      `vs2022 --vs2026` paths, and print whether probing was enabled (also reported in `RESULT:` JSON)
- [x] 1.3 Verify both generation modes on this platform: the Hybrid Makefile contains
      `-DENABLE_PROBING` with the flag (in the Hybrid block only) and does not without it
- [x] 1.4 Remove stale Hybrid objects when the flag flips (objects do not depend on the generated
      projects, so regenerating alone left the old prober compiled in); verified with both flips

## 2. Validation

- [x] 2.1 `generate --probing` + `compile --config Hybrid` (0 errors) + short editor run → 21 `[probe]` lines
      present
- [x] 2.2 `generate` (default) + clean `compile --config Hybrid` (0 errors) + short editor run (exit 0) →
      0 `[probe]` lines; tree left in this default state
- [x] 2.3 `format --dry-run` reports only the 2 pre-existing files (`particle_renderer.cpp`,
      `kinematic_platform.py`), none touched by this change

## 3. Documentation

- [x] 3.1 Update `openspec/kb/perf-probing-and-quality.md` to the opt-in workflow (regenerate after
      toggling, stale-object cleanup, don't forget `--probing`)
- [x] 3.2 Update `openspec/kb/dev-scripts.md` (`generate --probing`), `docs/PROFILING_MACOS_OPENGL.md`
      (§3 method + §7/§8 mention the flag) and the agent guides (`AGENTS.md`,
      `.github/copilot-instructions.md`)

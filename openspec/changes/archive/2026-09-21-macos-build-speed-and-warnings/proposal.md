# Proposal: macOS build speed and warning-free compilation

## Why

The macOS port currently pays two avoidable costs on every build:

1. **Noise.** A full Hybrid build emits **1619 clang warnings**; `compile.sh` hides them behind `-w`, so nobody sees them. Several are real defects, not cosmetics: `delete` through the abstract `z1::Event` base has a non-virtual destructor (undefined behaviour), 20 `ImGui::Text()` calls pass runtime strings as format strings (`%` in a name corrupts output), 5 `printf` specifiers do not match their argument types, and 16 RHI enum switches silently ignore `None`/sampler enumerators.
2. **Latency.** A cold Hybrid build takes **≈360 s** on the development machine (Intel i5-8257U, 4C/8T), and the build is **CPU-work-bound, not parallelism-bound**: `-j4` and `-j8` measure 361 s vs 358 s. Phase attribution shows the `runtime` project consumes **75 %** of the time (268 s), per-TU profiles show **≈67 % backend** (codegen/dwarf) vs ≈33 % frontend, and archived-size objects (`runtime` objects = 307 MB) are dominated by debug info. Measured A/B runs show the debug-info level is the single largest lever: `-gline-tables-only` cuts the runtime project by **17–28 %** (207→172 s back-to-back, 268→193 s in a longer run) while keeping line-level debugging, and shrinks its objects to 106 MB.

Because the engine is developed iteratively (edit → build → smoke/probe/capture loops) and the dev CLI drives long builds, build latency and a trustworthy warning baseline now gate day-to-day work.

## What Changes

**Warning hygiene (macOS, engine code)**

- `REFLECTED_FIELD` stops leaking `-Winvalid-offsetof` (1550 warnings): the `offsetof()` use on non-standard-layout reflected types is intentional and is scoped with a clang diagnostic push/pop inside the macro (no layout changes, MSVC unaffected).
- `FieldInfo::widget` gets an explicit default initializer, which removes all 411 `-Wmissing-field-initializers` warnings (the macro's aggregate initializer stops short of it).
- `z1::Event` gains a virtual destructor — fixes real UB when events are deleted polymorphically; subclasses inherit the fix.
- Editor/`type_field` code switches non-literal `ImGui::Text()` calls to `ImGui::TextUnformatted()` (20 sites, real format-string bug) and corrects 5 mismatched format specifiers.
- RHI enum conversions (16 sites) handle every enumerator instead of falling off the end of `switch`.
- Include casing fixed (`glfw/glfw3.h` → `GLFW/glfw3.h`), and `editor_layer.h` stops including `stb_image_write.h` in every editor TU.
- Engine projects (runtime, editor, game, bakery) compile with `-Wall -Wextra` on macOS, with exactly one documented noise suppression (`-Wno-unused-parameter`); everything else reported by the strict set is fixed. Third-party vendored headers are not edited; the single known `stb` deprecation is scoped to its two build TUs by a per-file suppression.
- Windows builds and MSVC projects are unchanged (all new flags are `system:macosx` scoped).

**Build speed (macOS)**

- macOS Hybrid (and Profile) builds use `-gline-tables-only` instead of `-g`: measured **−17 … −28 %** on the runtime project (roughly 75 % of total build time), objects shrink **307 MB → 106 MB**; line-level breakpoints still work in lldb/VS Code.
- `dev/z1.py compile` on macOS uses **ccache automatically when it is installed** (PCH-safe `CCACHE_SLOPPINESS=pch_defines,time_macros`), so repeat builds after branch switches, `git stash`, probing toggles (`generate --probing` wipes Hybrid objects by design) and test cycles hit the cache instead of recompiling. `--ccache` forces it, `--no-ccache` disables it; the RESULT line reports cache mode.
- `dev/z1.py compile` defaults to **all logical cores** on macOS (was hard-coded `-j4`), with a `--jobs N` override.
- `compile.sh` no longer forces `-w` (warnings are now fixed, not hidden).

**Verification of the two claims**

- A cold Hybrid build must get measurably faster than the recorded 361 s baseline (target: ≤ 300 s on the same machine) and the full build log must contain zero clang warnings for engine code.
- A second clean build with a warm ccache must complete in a small fraction of the cold time (cache-hit verification), and incremental `touch` builds must stay at or below the recorded 25 s baseline.

## Capabilities

### New Capabilities

- `macos-build-performance`: macOS build-speed requirements — debug-info mode for optimized configs, ccache integration in the dev CLI, job-count defaults, measured improvement floors, and Windows neutrality.
- `warning-free-build`: warning-hygiene requirements — zero warnings from engine code with the compiler's default warning set, strict (`-Wall -Wextra`) warning baseline for engine projects with a documented, minimal suppression list, and a rule that defect-indicating warnings are fixed rather than suppressed.

### Modified Capabilities

- (none) — the macOS compile-path contract (make invocation, clang diagnostic parsing, RESULT fields) is specified inside `macos-build-performance`. The existing `dev-scripts` spec is structurally invalid (pre-existing: missing `## Purpose`/`## Requirements`, requirements without modal verbs — 9 of 26 specs share this defect), so this change deliberately does not rebuild it; repairing those specs is a separate follow-up.

## Impact

- **Build system**: root `premake5.lua` (macOS-scoped warning/debug-info settings, new options), `engine/runtime|editor|game|bakery/premake5.lua`, regenerated Makefiles (gitignored).
- **Engine sources** (~15 files): `core/core.h`, `core/reflection.h`, `event/event.h`, `render/rhi/opengl_*.cpp`, `asset/material.cpp`, `3rdparty/imgui_layer.cpp`, `editor/editor_layer.cpp/.h`, `editor/type_field.cpp`, plus `-Wswitch` switch sites.
- **Dev CLI / wrappers**: `dev/commands/compile.py`, `dev/commands/_common.py` (helpers), `dev/z1.py` (help), `compile.sh`, `generate.sh`.
- **Docs/KB**: `openspec/kb/build.md` (macOS section: flags, ccache, jobs, baselines), `openspec/kb/dev-scripts.md` (new options).
- **Tooling**: optional Homebrew `ccache` install on the dev machine (auto-detected; build works without it).
- **No change** to Windows/MSVC behaviour, engine runtime behaviour, shaders, or assets.

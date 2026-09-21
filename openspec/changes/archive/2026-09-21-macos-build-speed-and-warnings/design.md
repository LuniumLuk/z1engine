# Design: macOS build speed and warning-free compilation

## Context

Baseline measurements (Intel i5-8257U 4C/8T 15 W, 8 GB, Apple clang 17, GNU Make 3.81, premake 5.0.0-beta8; Hybrid config; all numbers are wall clock on the same machine):

| Measurement | Value |
|---|---|
| Cold build `make config=hybrid -j4 game` | 361.5 s |
| Cold build `make config=hybrid -j8 game` | 358.4 s (no gain — CPU saturated) |
| Phase attribution (-j8) | third-party 28 s, **runtime 268 s (75 %)**, editor 42 s, game 20 s + 0.7 s link |
| Incremental: touch 1 `.cpp` / touch `render/global.h` | 25.1 s / 181.2 s |
| Per-TU (`-ftime-trace`) | frontend ≈ 33 %, **backend ≈ 67 %**; top TUs 20–68 s |
| Runtime objects / total objects | 307 MB / 401 MB |
| Debug-info A/B (runtime cold) | `-g` 268 s → `-gline-tables-only` **193 s (−28 %)** → `-g0` 180 s (−33 %); objects 307 MB → 106 MB → 62 MB |
| Confirmation A/B (back-to-back) | `-g` 207 s → `-gline-tables-only` **172 s (−17 %)** |
| Warnings (default clang) | 1619 (1550 `-Winvalid-offsetof`, 411 `-Wmissing-field-initializers` under `-Wall`-only paths, 20 `-Wformat-security`, 16 `-Wswitch`, 5 `-Wformat`, 5 non-portable include case, 3 polymorphic-delete, 2 vendored `stb` deprecations) |

Constraints: the repository is Windows-first (MSVC/VS2026 is the primary toolchain); macOS is a port that must not change Windows behaviour. `engine/3rdparty/` must not be edited. Coding style and OpenSpec workflow rules apply. The machine is a 15 W 4-core laptop: measurements drift with thermal state (observed ±25 % on identical cold builds), so comparisons must be same-machine and preferably back-to-back.

## Goals / Non-Goals

**Goals:**

- Zero clang warnings from engine code in a full macOS build, including a strict `-Wall -Wextra` baseline for engine projects with a deliberately tiny suppression list.
- Fix defect-indicating warnings (polymorphic delete UB, format-string misuse, format specifier mismatches, unhandled enum cases) rather than hiding them.
- Cut cold-build wall time on the dev machine from 361 s to ≤ 300 s and shrink object footprint substantially (debug-info lever), without losing line-level debuggability of Hybrid builds.
- Make repeat builds cheap when ccache is available, including the workflows that legitimately wipe objects (`generate --probing` toggles, branch switches).
- Keep the dev CLI the single entry point, with structured output that reports what the build actually did (jobs, ccache mode).
- Leave Windows/MSVC behaviour bit-identical.

**Non-Goals:**

- PCH for editor/game, unity builds, and switching to the premake `ninja` action — all plausible future levers, none measured as clearly worth the risk/churn here.
- Reducing frontend/header include cost (header-hygiene refactors) — backend dominates (67 %) and PCH already covers runtime.
- MSVC warning work on Windows, or touching third-party sources.
- Any runtime/engine behaviour change beyond the defect fixes that remove warnings.

## Decisions

**D1 — Use `-gline-tables-only` for macOS Hybrid and Profile builds.**

The A/B data shows the debug-info level is the dominant backend lever: line tables keep ~90 % of the achievable `-g0` win (193 s vs 180 s) while retaining file/line breakpoints, and the object footprint collapses (307 MB → 106 MB), which also speeds up `ar`/link and reduces disk I/O. Adding `buildoptions { "-gline-tables-only" }` after premake's `-g` is sufficient: verified that a later `-gline-tables-only` fully overrides `-g` (identical DWARF sections and object size to line-tables-only alone).
*Alternatives considered:* keep `-g` (status quo, no win); `-g0` in Hybrid (only 13 s better than line tables but removes all symbolic debugging — rejected); `-fno-standalone-debug` (unmeasured, subtle debug-info semantics — rejected for now); changing `symbols` per config (affects MSVC semantics — rejected).

**D2 — Provide a `--full-symbols` escape hatch at generate time.**

A premake option (`newoption { trigger = "full-symbols" }`) skips the line-tables build option for macOS Hybrid/Profile so a developer who needs full type information can regenerate without editing `premake5.lua`. `dev/z1.py generate --full-symbols` passes it through the existing option-plumbing pattern used by `--probing`.

**D3 — Enable `-Wall -Wextra` for engine projects on macOS with one suppression.**

After the planned fixes, the strict set's remaining signal is real (init-order bugs, sign-compare, unused lambda captures/fields, ignored qualifiers). `-Wno-unused-parameter` is suppressed project-wide because ~500 unnamed-parameter rewrites carry zero defect value; everything else reported must be fixed. Third-party projects keep the default warning set (their TUs are already clean).
*Alternatives considered:* default warnings only (leaves future regressions invisible); suppressing the noisy categories wholesale (hides signal); fixing unused parameters (churn without value).

**D4 — Handle the reflection macro's `offsetof` locally, not globally.**

`REFLECTED_FIELD` intentionally stores physical field offsets of non-standard-layout types; MSVC accepts this silently, clang warns. The macro wraps its `FieldInfo` construction in `_Pragma("clang diagnostic push/ignored(-Winvalid-offsetof)/pop")` (guarded so MSVC never sees it). This is a documented, single-site exception to "no suppression" because making the reflected types standard-layout is infeasible and the alternative offset computation is equally UB.

**D5 — Fix `FieldInfo::widget` (default initializer) instead of suppressing aggregate-init warnings.**

Giving `std::string widget;` an explicit default initializer removes all 411 `-Wmissing-field-initializers` warnings at the type level (clang only warns for fields without a default member initializer; the other trailing fields already have one). No behaviour change.

**D6 — Fix the polymorphic-delete UB in `z1::Event`.**

Add `virtual ~Event() = default;`. This silences the delete-abstract warnings and fixes real undefined behaviour (events are stored/deleted via base pointers).

**D7 — Wire ccache through the environment, macOS-only, auto-detected.**

The dev CLI exports `CC="ccache clang"`, `CXX="ccache clang++"` and `CCACHE_SLOPPINESS="pch_defines,time_macros"` (required for the runtime precompiled header) when `ccache` is on `PATH`; premake's generated Makefiles keep environment-provided `CC`/`CXX` (they only substitute when the variable origin is `default`). `--ccache` forces, `--no-ccache` disables; the RESULT JSON and status lines report which mode ran. ccache remains optional: without it, builds behave exactly as today.
*Alternatives considered:* modifying generated Makefiles (ephemeral, regenerated by premake); shim scripts on PATH (implicit, surprising); premake toolset wrappers (fiddly, affects VS projects).

**D8 — Default to all logical cores on macOS.**

`dev/z1.py compile` currently hard-codes `-j4`. The macOS path will use `os.cpu_count()` unless `--jobs N` is given. On the dev machine this measures neutral (361 s vs 358 s) — the change targets bigger Macs, and the printed elapsed makes it verifiable. Windows behaviour is untouched (devenv manages its own parallelism).

**D9 — Stop hiding warnings in `compile.sh`.**

The wrapper's `CFLAGS/CXXFLAGS=-w` was a port-era crutch; with a clean baseline it must go, otherwise the new warning baseline is unobservable through the script most users run.

**D10 — Verification protocol.**

Record baselines and post-change numbers in `openspec/kb/build.md`; compare same-machine cold builds (≤ 300 s target), log-based warning counts (must be 0 for engine code), a warm-ccache clean rebuild (cache-hit evidence), and incremental `touch` timings (≤ 25 s baseline). Because thermal state moves absolute numbers by up to 25 %, comparative back-to-back runs are the primary evidence and thresholds are conservative.

## Risks / Trade-offs

- [Hybrid debug fidelity drops from full types to line tables] → Debug config keeps full `-g`; `generate --full-symbols` restores the old flags for macOS Hybrid/Profile; documented in the KB.
- [ccache + precompiled headers can silently miss or error] → `CCACHE_SLOPPINESS=pch_defines,time_macros` is set by the dev CLI; verification must show hit counts after a clean rebuild; if PCH interaction proves broken, fall back to wrapping only non-runtime projects or dropping ccache for the runtime project.
- [Disk usage from ccache (multi-GB with 100 MB TUs)] → document `ccache -M` sizing and `ccache -C` in the KB; default cache location is per-user.
- [`-Wall -Wextra` may surface warnings in code paths not covered by the measurement (e.g. bakery) ] → the implementation captures a full strict build log; the fix list is bounded by measured categories, and any new category is triaged (fix or explicitly documented suppression).
- [Thermal/measurement noise makes regressions detectable only as trend] → back-to-back A/B protocol, fixed workload (`--clean hybrid`, `game` target), numbers recorded in the KB.
- [Enabling line tables changes build flags for Profile too — could surprise profiling workflows that want variable inspection] → `--full-symbols` covers it; Profile keeps `-gline-tables-only` for consistency with Hybrid.

## Migration Plan

1. Land premake/source/dev-CLI changes together (single change); regenerate projects (`dev/z1.py generate`) and rebuild from clean.
2. Rollback = revert the change's files and regenerate; ccache is additive and inert if unused (remove with `ccache -C` / uninstall).
3. KB (`build.md`, `dev-scripts.md`) updated in the same change; no runtime data migration.

## Open Questions

- Whether the end-to-end cold build lands comfortably under 300 s (the runtime-project delta is measured; the full-build confirmation run is recorded during verification and the KB baseline is updated with the actual number).
- Whether `ccache` should be installed by default on port-machines (documented as optional; the build works without it).

# Tasks: macOS build speed and warning-free compilation

## 1. Reflection macro warning fixes (1550 + 411 warnings)

- [x] 1.1 `engine/runtime/source/core/core.h`: wrap the `FieldInfo` construction in `REFLECTED_FIELD` with a clang-guarded `_Pragma("clang diagnostic push")` / `ignored "-Winvalid-offsetof"` / `pop` pair; add a one-line comment stating why the macro needs `offsetof()` on non-standard-layout types
- [x] 1.2 `engine/runtime/source/core/reflection.h`: give `FieldInfo::widget` an explicit default member initializer (`= {}`) so the macro's aggregate initializer no longer triggers `-Wmissing-field-initializers` (also applied to `TypeInfo::fields`)
- [x] 1.3 Verify with a runtime rebuild that `-Winvalid-offsetof` and `-Wmissing-field-initializers` are gone (grep the build log)

## 2. Defect fixes behind warnings

- [x] 2.1 `engine/runtime/source/event/event.h`: add `virtual ~Event() = default;` (fixes polymorphic-delete UB and the `-Wdelete-*-non-virtual-dtor` warnings); the same fix was applied to `GraphicsContext` in `engine/runtime/source/render/graphics_context.h`
- [x] 2.2 `engine/editor/source/editor_layer.cpp` + `engine/editor/source/type_field.cpp`: replace `ImGui::Text(<runtime string>)` with `ImGui::TextUnformatted(...)` (20 sites); fix the 5 mismatched format specifiers (`%llu` → `%zu`, `%d` → `%zu` for `size_type`)
- [x] 2.3 `engine/runtime/source/render/rhi/opengl_shader.cpp`, `opengl_pipeline.cpp`, `opengl_image.cpp`, `opengl_framebuffer.cpp`, `opengl_buffer.cpp`, `engine/runtime/source/asset/material.cpp`: make the enum switches handle all enumerators (add the missing cases to the existing fallback paths rather than silencing them)
- [x] 2.4 Rebuild and confirm zero remaining `-Wformat-security`, `-Wformat`, `-Wswitch` and delete-dtor warnings from engine sources

## 3. Include hygiene and vendored-header scoping

- [x] 3.1 `engine/runtime/source/3rdparty/imgui_layer.cpp` (and any other site flagged): fix `glfw/glfw3.h` → `GLFW/glfw3.h` casing (`-Wnonportable-include-path`) — 5 sites fixed (imgui_layer, input, window, opengl_context, prober)
- [x] 3.2 `engine/editor/source/editor_layer.h` → `.cpp`: move `#include "stb/stb_image_write.h"` out of the header into the translation unit that uses it
- [x] 3.3 `engine/runtime/premake5.lua` + `engine/bakery/premake5.lua`: add a macOS-scoped, per-file `buildoptions { "-Wno-deprecated-declarations", "-Wno-missing-field-initializers" }` filter for `**/stb_build.cpp` so the vendored diagnostics are contained without editing `engine/3rdparty/`
- [x] 3.4 Regenerate + rebuild; confirm the only warnings left in `engine/3rdparty/` originate from the two stb build TUs (and are suppressed there)

## 4. Strict warning baseline for engine projects

- [x] 4.1 `premake5.lua` (or per-project premake files): add a `system:macosx` filter enabling `-Wall -Wextra` and the documented `-Wno-unused-parameter` for `runtime`, `editor`, `game`, `bakery` only (each suppression carries an explanatory comment)
- [x] 4.2 Regenerate, rebuild from clean, capture the strict log, and fix every remaining engine-code warning category (`-Wreorder-ctor`, `-Wsign-compare`, `-Wunused-variable`, `-Wunused-function`, `-Wunused-private-field`, `-Wunused-lambda-capture`, `-Wignored-qualifiers`)
- [x] 4.3 Confirm third-party projects still compile with the default warning set and stay clean

## 5. Debug-info mode for macOS optimized builds

- [x] 5.1 Root `premake5.lua`: add `newoption { trigger = "full-symbols" }` and a `system:macosx` filter adding `buildoptions { "-gline-tables-only" }` for Hybrid/Profile unless `--full-symbols` was passed
- [x] 5.2 `dev/commands/generate.py` + `dev/z1.py`: pass `--full-symbols` through to premake (same plumbing pattern as `--probing`) and document it in the command list
- [x] 5.3 Regenerate and verify the generated Makefiles: Hybrid/Profile `ALL_CXXFLAGS` end with `-O2 -g -gline-tables-only …` (verified that the later `-gline-tables-only` fully overrides `-g` — identical DWARF sections/object size to line-tables-only alone); Debug keeps `-g`; `generate --full-symbols` restores `-O2 -g`
- [x] 5.4 Rebuild from clean and time it; record the cold-build wall clock (target ≤ 300 s vs the 361.5 s baseline) and the runtime object-directory size (target < 200 MB vs 307 MB) — **250.1 s**, 106 MB (see Evidence)

## 6. Dev CLI build speed: jobs + ccache

- [x] 6.1 `dev/commands/compile.py`: default macOS jobs to `os.cpu_count()`, add `--jobs N` (validated, exit code 4 on bad input), keep Windows behaviour untouched
- [x] 6.2 `dev/commands/compile.py` (+ `dev/commands/_common.py` helper): detect `ccache` on `PATH` and run make with `CC="ccache clang"`, `CXX="ccache clang++"`, `CCACHE_SLOPPINESS="pch_defines,time_macros"`; add `--ccache` (hard fail with exit 4 when missing) and `--no-ccache`; report the cache mode in the status output and the `RESULT:` summary
- [x] 6.3 `compile.sh`: remove the `-w` injection so warnings stay visible
- [x] 6.4 Verify: `python dev/z1.py compile --config Hybrid` on a clean tree passes with 0 errors / 0 warnings and reports jobs + cache mode; `--no-ccache`, bad `--jobs` values, `--ccache` without ccache and conflicting flags all fail/behave per spec (exit code 4)

## 7. ccache installation and cache effectiveness

- [x] 7.1 Install ccache on the dev machine — **blocked in this environment**: Homebrew tried to build OpenSSL/CMake from source (network too slow; aborted), the cached Sonoma bottle links dylibs that are not installed (blake3/xxhash/hiredis/openssl@3), and GitHub release downloads time out. The dev-CLI wiring was verified end-to-end with a passthrough `ccache` stub: auto-detection, `CC="ccache clang"`/`CXX="ccache clang++"`, `CCACHE_SLOPPINESS=pch_defines,time_macros`, RESULT reporting, and `--no-ccache` disabling all observed. User install step: `brew install ccache`
- [x] 7.2 Prove effectiveness: stub run recorded every compile/link invocation (66 calls in the full-rebuild run); the warm-cache wall-clock measurement remains **pending a real ccache install** (documented follow-up, not fabricated)
- [x] 7.3 Record `ccache -s` sizing guidance (max-size, `ccache -C`) for the KB — added to `openspec/kb/build.md`

  > Follow-up once ccache is installed: `ccache -M 10G`, inspect `ccache -s`, and append the measured warm-cache clean-build time to the KB table

## 8. Documentation and knowledge base

- [x] 8.1 `openspec/kb/build.md`: add the macOS build section (toolchain, make invocation, debug-info mode, `--full-symbols`, jobs default, ccache usage/escape hatches, measured baselines and protocol)
- [x] 8.2 `openspec/kb/dev-scripts.md`: document the new `compile` options (`--jobs`, `--ccache`, `--no-ccache`) and `generate --full-symbols`
- [x] 8.3 `openspec/kb/index.md`: update the touched page summaries (build.md/dev-scripts.md entries now mention the macOS settings)

## 9. Final verification

- [x] 9.1 `python dev/z1.py generate` (default) and `python dev/z1.py generate --probing` both succeed; `generate --probing` re-wipes Hybrid objects by design (verified: the probing rebuild completed with 0 errors / 0 warnings, 268.4 s, and smoked at 3.9 s; the default regeneration afterwards rebuilt probe-free in 256.5 s, 0/0, and the binary exits cleanly)
- [x] 9.2 `python dev/z1.py compile --config Hybrid`: **0 errors, 0 warnings** on a clean build; logs `/tmp/z1_final_clean.log` and `/tmp/z1_timings.txt` (250.1 s clean build)
- [x] 9.3 `python dev/z1.py validate-shaders`: 194 passed / 9 failed — all failures are the pre-existing Apple 16-sampler fragment limit (`sprite_2d_batched.glsl`); no shader was touched by this change
- [x] 9.4 `python dev/z1.py smoke --frames 10`: 3.6 s from a tty (renders 10 frames, exits cleanly). From a VS Code task shell it times out because stdin is not a tty (pre-existing macOS automation trap documented in the repo notes, not a regression)
- [x] 9.5 `python dev/z1.py dcv --auto`: generate ✓, compile ✓ (0 errors/0 warnings), format ✓, validate-shaders skipped (no .glsl changes), tests skipped (no macOS test binaries — pre-existing port gap), then **stops at `benchmark`** because the step expects `engine/bin/Debug/game.exe` (Windows-shaped mapping; pre-existing macOS gap). The applicable gates (compile + smoke + shaders) were run manually and pass as recorded above
- [x] 9.6 Timed evidence recorded in `openspec/kb/build.md`: clean **250.1 s** (≤ 300 s ✓), incremental 1 cpp **18.9 s** (≤ 25 s ✓), wide header cascade **133.7 s**, no-op **1.4 s**; warm-ccache measurement pending a real ccache install (see 7.2)
- [x] 9.7 `python dev/z1.py format --dry-run` shows no drift in the modified files; `git status` lists only intended files (the format step auto-fixed one pre-existing drift file, `engine/content/scripts/kinematic_platform.py`, which was reverted)

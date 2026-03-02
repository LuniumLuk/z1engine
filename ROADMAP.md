z1engine Roadmap

Overview
- Purpose: guide short- and medium-term development based on current codebase, recent commits, and existing features.
- Structure: milestone-based checklist with tasks that are concise, extendable, and owner-assignable.

How to use
- Edit this file to add/remove tasks; prefix unchecked boxes with "- [ ]" and checked with "- [x]".
- Add sub-tasks by indenting a new checkbox line below a parent task.
- For larger efforts, create an ISSUE and reference it from the task line.

Milestones

1) Project hygiene and CI
- [ ] Ensure reproducible build on supported Windows VS versions
  - Details: verify generate_vs2022.bat and build_vs2022.bat; add script-based smoke build job.
- [ ] Add an automated test runner for test/ projects
  - Details: add CI step to build & run test executables and fail on non-zero exit.
- [ ] Add basic lint/format step (opt-in) and document local dev steps
  - Details: discuss formatter choice (clang-format) and decide in ISSUE before enforcement.

2) Stabilize runtime & core systems
- [ ] Stabilize RenderGraph and renderer integration
  - Details: ensure render graph execution ordering, resource lifetimes and simple passes (forward, postprocess, taa) work robustly.
- [ ] Ensure asset manager correctness and GUID handling
  - Details: validate binary asset formats and importer/exporter round trips for common assets (meshes/textures/materials).
- [ ] Harden window/input/timer systems for edge cases (minimized, focus loss)
  - Details: add tests and guarded behavior when window size is zero or input blocks.

3) Asset pipeline & bakery
- [ ] Finish and document bakery workflows (image compression, shader baking)
  - Details: add CLI helpers for batch baking; document expected file layout under engine/ and asset/.
- [ ] Add automated asset validation in CI for a small sample set
  - Details: run bakery tools to produce sample engine assets and compare checksums.

4) Editor and UX improvements
- [ ] Improve ContentBrowser usability and drag-drop workflows
  - Details: streamline importing, previewing, and creating material/mesh instances.
- [ ] Stabilize scene serialization and editor undo/redo basics
  - Details: make transient vs persistent entity behavior clear; add tests for scene save/load.
- [ ] Add editor unit/integration tests for core editor components
  - Details: run in headless mode when possible or stub systems for CI.

5) Rendering features and quality
- [ ] Finalize PBR material and texture assignment pipeline
  - Details: ensure glTF importer maps textures and material instances correctly; add sample scenes that exercise PBR.
- [ ] Improve postprocessing stack (TAA, tone mapping, FXAA/AA options)
  - Details: add toggles and default presets, verify with sample scenes.
- [ ] Performance profiling and bottleneck reduction
  - Details: add profiling traces across frames in CI or nightly runs and capture metrics for major subsystems.

6) Scripting and extensibility
- [ ] Polish Python layer embedding and scripting API
  - Details: define stable surface area for script-driven entity behaviors and editor automation.
- [ ] Add example scripts and documentation for common tasks
  - Details: include script examples under test/ or scripts/ showing lifecycle hooks.

7) Packaging, releases, and documentation
- [ ] Document release packaging steps and create reproducible ZIP/installer
  - Details: use release/ folder conventions and automate packaging with a script.
- [ ] Expand developer documentation (getting started, code style, contribution guide)
  - Details: link to AI_AGENT_SETUP.md and AI_AGENT_CODING_STYLE.md; add BUILD.md.

8) Tests, tooling, and observability
- [ ] Create regression test harness for render outputs (image diffs)
  - Details: render deterministic scenes and compare to golden images with tolerance.
- [ ] Integrate basic metrics and health endpoints for long-running editor/runtime processes
  - Details: allow remote capture of CPU/GPU usage and frame time metrics.

Technical debt and housekeeping
- [ ] Audit 3rdparty libraries for license and update needs
- [ ] Remove dead code and unused assets incrementally (document removals)

Next actions (immediately actionable)
- [ ] Add CI smoke build that runs generate_vs2022.bat and builds the solution
- [ ] Pick 3 high priority bugs from recent issues and create tasks with owners
- [ ] Add a CONTRIBUTING.md referencing AI agent docs and the coding style file

Notes
- Prioritize tasks that unblock contributors and reduce friction (build, tests, docs) before large feature work.
- Break large items into tracked issues with clear acceptance criteria.


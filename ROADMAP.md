z1engine Roadmap

Overview
- Purpose: guide short- and medium-term development based on current codebase, recent commits, and existing features.
- Structure: Phase-based roadmap prioritizing stability, asset pipeline, and modern rendering features.

How to use
- Edit this file to add/remove tasks; prefix unchecked boxes with "- [ ]" and checked with "- [x]".
- Add sub-tasks by indenting a new checkbox line below a parent task.

Phase 1: Foundation & Core Stability (Current)
- [ ] **Infrastructure & CI**
  - [ ] Standardize `generate_vs24.bat` (or latest) and `build.bat` for reproducible builds.
  - [ ] Implement integrated CTest/test runner for all `test/` projects.
  - [ ] Add `clang-format` configuration and enforce via pre-commit or CI.
- [ ] **Core Systems Hardening**
  - [ ] Robust error handling for RHI (OpenGL) and state management.
  - [ ] Formalize `GUID` system for asset referencing versus file paths.
  - [ ] Implement thread-safe Task System for background loading and bakery processing.

Phase 2: Asset Pipeline & Bakery (Critical Path)
- [ ] **Comprehensive Bakery Development**
  - [ ] Expand `baker/image` to support BC7/ASTC compression (using ISPC/Compressonator).
  - [ ] Implement `baker/mesh` to convert `.obj`/`.gltf` to internal binary format.
  - [ ] Implement `baker/shader` for cross-platform bytecode (SPIR-V) and variant management.
- [ ] **Asset Management**
  - [ ] Implement asynchronous asset loading with reference counting.
  - [ ] Add hot-reloading support for textures, shaders, and materials.

Phase 3: Render Graph & Modern Rendering
- [ ] **Render Graph Evolution**
  - [ ] Transition `renderer_forward` and `renderer_2d` to use `RenderGraph` nodes exclusively.
  - [ ] Implement automatic resource transition barrier generation (even for OpenGL).
  - [ ] Support transient resource aliasing to minimize VRAM footprint.
- [ ] **High-End Features**
  - [ ] Implement PBR Pipeline (Physically Based Rendering) with IBL (Image Based Lighting).
  - [ ] Develop a standard Post-Processing stack (Bloom, Tonemapping, TAA, FXAA).
  - [ ] Shadow Mapping system (Cascaded Shadow Maps for directional lights).

Phase 4: Editor & Tooling UX
- [ ] **Editor Productivity**
  - [ ] Integrated `ContentBrowser` with thumbnail generation and drag-and-drop.
  - [ ] Undo/Redo system using Command pattern for scene editing.
  - [ ] Proper `PickingSystem` implementation for 3D viewport selection.
- [ ] **Python Integration**
  - [ ] Expose full Scene/Entity API to `python_layer`.
  - [ ] Create editor automation scripts (e.g., batch asset importing).

Phase 5: Performance & Observability
- [ ] **Profiling & Metrics**
  - [ ] Integrate Tracy Profiler for deep instrumentation of CPU/GPU.
  - [ ] In-game debug overlay for frame timings, draw calls, and memory usage.
- [ ] **Optimization**
  - [ ] Implement Draw Call Batching / Instancing for static and dynamic meshes.
  - [ ] Level of Detail (LOD) system and frustum/occlusion culling.

Technical Debt & Housekeeping
- [ ] **3rdparty Cleanup**: Audit and unify versions of vendored libraries.
- [ ] **Documentation**: Maintain `AI_AGENT_SETUP.md` and create `BUILDING.md`.
- [ ] **Shader cleanup**: Migrate raw GLSL strings to external `.glsl` files.

Next Immediate Actions
- [ ] Fix `test/test_render_graph.cpp` integration with latest `RenderGraph`.
- [ ] Finalize `bakery` CLI interface for batch processing.
- [ ] Add `CONTRIBUTING.md` with coding standards.


# Architecture
> Summary: High-level system structure of z1engine -- modules, entry points, data flow
> Scope: engine/runtime/, engine/editor/, engine/bakery/, engine/game/

## Primary Components

| Component | Path | Role |
|-----------|------|------|
| **Runtime** | `engine/runtime/` | Core engine library: rendering, ECS, assets, scripting |
| **Editor** | `engine/editor/` | ImGui-based development environment, panels, gizmos |
| **Bakery** | `engine/bakery/` | Offline asset processing CLI (textures, meshes, shaders) |
| **Game** | `engine/game/` | Standalone game executable, links runtime |

## Runtime Modules

| Module | Path | Key Types |
|--------|------|-----------|
| `core/` | `engine/runtime/source/core/` | `Application`, `LayerStack`, `Window` (GLFW) |
| `scene/` | `engine/runtime/source/scene/` | `Scene`, `Entity`, components, systems |
| `render/` | `engine/runtime/source/render/` | `RenderGraph`, `RenderGraphNode`, `Shader`, RHI |
| `asset/` | `engine/runtime/source/asset/` | `AssetManager`, `Asset<T>`, `BinaryFile`, importers |
| `event/` | `engine/runtime/source/event/` | Event system, `BIND_EVENT_FN` |
| `python/` | `engine/runtime/source/python/` | `PythonLayer`, `PythonScript`, `z1` module bindings |
| `animation/` | `engine/runtime/source/animation/` | Skeletal animation |
| `util/` | `engine/runtime/source/util/` | Math, profiling, reflection |

## Data Flow

- `Application` runs the main loop, updating `LayerStack`
- `GameLayer` updates the active `Scene` (ECS tick)
- `Scene::on_update` iterates entities via EnTT registry
- Render passes execute through `RenderGraph` node dependencies
- `AssetManager` loads assets lazily by GUID, caching in memory
- `Bakery` runs offline: reads raw files, writes optimized binary

## Build System

- **Premake5** generates VS2026 `.sln`/`.vcxproj` files
- Root `premake5.lua` defines all projects and test auto-discovery
- `utils/premake/premake5.exe vs2022 --vs2026` generates projects

-> see [build.md]
-> see [render-pipeline.md]
-> see [ecs.md]

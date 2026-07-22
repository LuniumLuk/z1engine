## Why

The reflection system (`engine/runtime/source/core/reflection.h`, macros in `engine/runtime/source/core/core.h`) is small and effective, but it is under-used: only 11 structs and 4 enums are reflected. Types that are hand-maintained everywhere instead — `ScriptComponent` script entries, `StaticMeshComponent`/`SkeletalMeshComponent` asset refs, `TagComponent::m_id`, `Material`/`MaterialInstance` variables, `Scene::EditorCameraData` — each require hand-written editor UI, hand-written scene YAML, and hand-written pybind glue.

Three gaps cause recurring manual work and drift:

- `FF_Serializable` exists but nothing reads it: scene YAML in `engine/runtime/source/scene/scene.cpp` is fully hand-written per component, so adding one reflected field means touching the header, the editor, the serializer, and the bindings separately.
- `utils/gen_pybinds.py` is not wired into the build, so `engine/runtime/source/python/py_engine.gen.cpp` is already stale (missing `ParticleComponent`, `RenderMode`, `EmitterShape`, `ParticleBlendMode` bindings).
- The editor inspector (`engine/editor/source/type_field.cpp`) still hand-codes UI for mesh/camera/script components and the add/remove-component menu, so reflected fields do not surface automatically.

## What Changes

- Keep the current design unchanged at its core: `REFLECTED_STRUCT` / `REFLECTED_FIELD` / `REFLECT_ENUM` macros, static registration, `TypeRegistry` / `EnumRegistry`, offset-based `FieldInfo`. All upgrades are incremental additions, no framework replacement.
- Extend the core minimally:
  - Optional type-erased hooks on `TypeInfo` (default-construct; for components: add/remove/has on `Entity`) via a new `REFLECTED_COMPONENT` macro, so YAML loading, the editor component menu, and Python `add_component` stop keeping separate hand-written type lists.
  - Field support for `Guid` and asset references (`std::shared_ptr<T>` where `T` is an engine asset type), plus optional custom get/set accessors for fields that do not fit the offset+typeid model (tagged-union values, script entry lists).
- Reflect the structs that need it (criteria: shown in the editor, serialized, or useful to Python scripts): `TagComponent::m_id`, `StaticMeshComponent::m_mesh`, `SkeletalMeshComponent::m_mesh`, `ScriptComponent` script entries, `Material::Variable` / `Material::m_variables` / `MaterialInstance::m_override_variables` (with `DataType` enum), `Scene::EditorCameraData`.
- Implement reflection-driven YAML (de)serialization honoring `FF_Serializable` for components, `GlobalSettings`, and `EditorCameraData`, keeping the current scene file schema compatible, and migrate the hand-written blocks in `scene.cpp` to it.
- Make Python binding generation automatic and seamless:
  - New `python dev/z1.py gen-pybinds` command wrapping `utils/gen_pybinds.py`, integrated into `generate`/`dcv` with a freshness gate so stale generated bindings fail validation.
  - Extend the generator for the new field kinds (Guid, asset refs, `std::array`/`std::vector` of reflected types, script entries) and regenerate `py_engine.gen.cpp` + `engine/stubs/z1.pyi`.
  - Expose minimal keyboard/mouse input (polling + key constants) to Python so real games can be scripted.
- Upgrade the editor to be fully reflection-driven: replace hand-written mesh/camera/script inspector sections with the generic field UI, generate the add/remove-component menu from `TypeRegistry`, generalize the asset-reference widget, and drive the material inspector from reflected `Material` data.
- Write a simple demo game in Python under the game content root (`content/`, scene `demo_scene` as already referenced by `run_game.bat`) using default engine assets (engine meshes, textures, materials), demonstrating the reflected Python APIs.

### Deferred

The following are documented as future work but not in scope for this change:

- Method reflection, inheritance graphs, and constructor/destructor reflection beyond the component attach hooks.
- Reflecting renderer-internal structs (`GlobalConstants`, `PerFrameConst`, pipeline/framebuffer descriptions) and bakery/offline types.
- Prefab-system reflection beyond what component serialization already covers.
- Python hot-reload of scripts and any migration to an external reflection library (rttr, entt::meta).

## Capabilities

### New Capabilities

- `reflection-system`: the core metadata/registry, the reflected-type coverage across the engine, and reflection-driven scene serialization with a stable YAML schema.
- `python-binding-generation`: build-integrated generation of pybind11 bindings and `z1.pyi` stubs from reflected types, with a freshness gate and the Python scripting surface (including input).
- `editor-reflection-inspection`: fully reflection-driven entity inspector, add/remove-component menu, asset-reference and script-entry widgets, and material editing.
- `python-demo-game`: a runnable demo game scene plus Python scripts demonstrating the reflected Python API surface using default engine assets.

### Modified Capabilities

- `dev-scripts`: the unified CLI gains a `gen-pybinds` command, and `dcv` gains a generated-bindings freshness check.

## Impact

- `engine/runtime/source/core/reflection.h`, `engine/runtime/source/core/reflection.cpp`, `engine/runtime/source/core/core.h` — incremental core extensions (hooks, Guid/asset/custom-accessor fields, `REFLECTED_COMPONENT`).
- `engine/runtime/source/scene/component/base.h`, `mesh.h`, `engine/runtime/source/asset/material.h`, `engine/runtime/source/render/data_types.h`, `engine/runtime/source/scene/scene.h` — new `REFLECTED_*` declarations.
- `engine/runtime/source/scene/scene.cpp` — hand-written YAML blocks replaced by the reflection-driven serializer; `engine/runtime/source/util/yaml.h` extended as needed.
- `utils/gen_pybinds.py`, `engine/runtime/source/python/py_engine.gen.cpp`, `engine/runtime/source/python/py_engine.cpp`, `engine/stubs/z1.pyi` — generator upgrades, input bindings, regenerated outputs.
- `dev/commands/gen_pybinds.py` (new), `dev/z1.py`, `dev/commands/generate.py`, `dev/commands/dcv.py` — CLI integration and freshness gate.
- `engine/editor/source/type_field.cpp`, `engine/editor/source/editor_layer.cpp` (and material inspector panel) — reflection-driven UI.
- `engine/test/test_scene_serialize.cpp`, `engine/test/test_pybind11.cpp`, new reflection tests — coverage for round-trip and bindings.
- `content/scene/demo_scene.yaml`, `content/scripts/*.py` (new game content root) — the demo game.
- `openspec/kb/ecs.md`, `openspec/kb/python-scripting.md`, `openspec/kb/dev-scripts.md` — KB updates at completion.

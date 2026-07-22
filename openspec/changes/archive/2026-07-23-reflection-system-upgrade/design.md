## Context

The engine has a hand-rolled, intentionally minimal reflection system: macros in `engine/runtime/source/core/core.h` register types at static-init time into `TypeRegistry` / `EnumRegistry` (`engine/runtime/source/core/reflection.h`). Consumers are the editor inspector (`engine/editor/source/type_field.cpp`), the pybind generator (`utils/gen_pybinds.py`), and — by intent only — serialization (`FF_Serializable` is defined but unused; scene YAML is hand-written in `engine/runtime/source/scene/scene.cpp`).

Pain points measured against the current tree:

- Adding a persistent component field today means editing up to four places: the header macro, `scene.cpp` save/load blocks, possibly `type_field.cpp`, and nothing updates Python until someone manually re-runs `gen_pybinds.py` (not wired into any build step; `py_engine.gen.cpp` is currently stale — no `ParticleComponent`/`RenderMode` bindings).
- The editor hand-codes UI for `StaticMeshComponent`, `SkeletalMeshComponent`, `CameraComponent`, `ScriptComponent` and the add/remove-component menu (`editor_layer.cpp:462-472`).
- `Material`/`MaterialInstance` variables, `ScriptComponent` entries, and `TagComponent::m_id` are serialized and displayed entirely by hand.

The change must stay incremental per the project goal: no new dependency, no redesign, keep the macro/static-registration model.

## Goals / Non-Goals

**Goals:**

- One source of truth: a reflected field automatically appears in the editor inspector, the scene YAML, and the Python bindings (after codegen), with at most a widget-hint string as extra metadata.
- Reflect the engine structs that are currently hand-maintained (component asset refs, script entries, material variables, tag id, editor camera data).
- Wire binding generation into `python dev/z1.py` so generated code can never silently go stale.
- Editor inspector and add/remove-component menu become fully reflection-driven.
- Ship a runnable Python demo game proving the reflected API surface with default engine assets.

**Non-Goals:**

- Method/property reflection, inheritance modeling, or replacing the registry with rttr/entt::meta.
- Reflecting renderer-internal or bakery/offline types.
- Changing the scene YAML schema (same keys, same structure — migrations must round-trip existing scenes).
- A generic prefab/UI binding system beyond what component serialization already covers.

## Decisions

### Keep the macro + static-registration core

**Decision**: Retain `REFLECTED_STRUCT` / `REFLECTED_FIELD` / `REFLECT_ENUM`, the `TypeRegistry`/`EnumRegistry` singletons, and offset-based `FieldInfo` exactly as they are. New capabilities are added as optional fields/hooks on the existing structs.

**Rationale**: The system is ~235 lines, has zero dependencies, and already feeds the editor and codegen. The stated goal is an incremental upgrade, not a rewrite.

**Alternative considered**: Adopt `entt::meta` (EnTT is already linked). Rejected: it duplicates the existing registry, changes every declaration site, and buys nothing the incremental extensions below don't provide.

---

### Add optional type-erased hooks to `TypeInfo`; new `REFLECTED_COMPONENT` macro

**Decision**: `TypeInfo` gains optional `std::function` hooks: `construct()` (default-construct into a buffer) and, for components, `add_to(entity)` / `remove_from(entity)` / `has_in(entity)`. A new `REFLECTED_COMPONENT(Type)` macro in `core/core.h` expands to `REFLECTED_STRUCT(Type)` plus hook registration using `Entity`'s public API. Plain `REFLECTED_STRUCT` remains for non-component types.

**Rationale**: This one hook set eliminates three separate hand-maintained type lists: the per-component YAML dispatch in `scene.cpp`, the add/remove-component menu in the editor, and the hand-written `add_static_mesh`/`add_camera`/... lambdas in `py_engine.cpp` (replaced by a generic `entity.add_component("Name")`).

**Alternative considered**: Keep per-system type lists (visitor in each consumer). Rejected: every new component would still require edits in N places — the exact manual work this change removes.

---

### Reflection-driven scene serialization honoring `FF_Serializable`

**Decision**: Add generic `serialize_field`/`deserialize_field` + per-type loops (in `engine/runtime/source/scene/serialization.{h,cpp}` or inside `scene.cpp`) that walk `TypeInfo` fields, honor `FF_Serializable`, use existing `util/yaml.h` glm/Guid converters, and derive YAML keys from field names exactly as today (strip `m_`, snake_case). Migrate `Scene::load`/`Scene::save` component blocks, `global_settings`, and `editor_camera` to it. Fields whose hand-written YAML keys differ from the derived name, or whose values need special handling (script entries, asset refs by GUID), are handled by small per-field adapters registered alongside the field — not by keeping monolithic hand-written blocks.

**Rationale**: Makes `FF_Serializable` real, deletes ~300 lines of parallel hand-written code, and makes "reflect a field → it persists" true. `engine/test/test_scene_serialize.cpp` round-trip plus loading the existing `engine/content/scene/ParticleDemo.yaml` unchanged guard schema compatibility.

**Alternative considered**: Keep hand-written YAML and extend it for each newly reflected field. Rejected: that is the drift source; the hand-written path already skips reflected fields today (e.g. `AnimationComponent::current_time`, `SpriteComponent::m_extras`).

---

### Represent asset references as reflected `std::shared_ptr<T>` fields with an asset-type trait

**Decision**: Reflect asset-reference fields (`StaticMeshComponent::m_mesh`, `SkeletalMeshComponent::m_mesh`, sprite/skylight textures already reflected) as plain `shared_ptr<T>` fields. A single `is_asset_type<T>` trait (explicit list in `reflection.h`: `Texture2D`, `StaticMesh`, `SkeletalMesh`, `Animation`, `Material`, `MaterialInstance`, `Shader`) lets every consumer recognize them: editor renders the generalized `ASSET_ITEM` drag-drop widget (today special-cased per type in `type_field.cpp`), YAML stores the asset GUID/path as today, pybind exposes a path string getter/setter resolved through `AssetManager`.

**Rationale**: One trait in one header; all three consumers stop special-casing asset types.

**Alternative considered**: Introduce a `Guid`-typed asset handle field and refactor components to store handles. Rejected: a wider refactor of component/runtime code for no behavioral gain; the trait achieves the same with additive metadata.

---

### Optional custom get/set accessors on `FieldInfo` for non-layout fields

**Decision**: `FieldInfo` gains an optional pair of type-erased `get`/`set` callbacks used when a field cannot be modeled as offset+typeid+size: `Material::Variable::Value` (tagged union keyed by `DataType`) and `ScriptComponent`'s script entry list (module/class strings mirrored from the runtime `ScriptData` vector). When present, editor/serializer/generator use the callbacks; otherwise the existing offset path is used. `DataType` is registered via `REFLECT_ENUM`.

**Rationale**: ~30 lines of core addition unlocks the two largest remaining hand-written surfaces (material inspector/YAML, script component) without distorting their runtime data structures.

**Alternative considered**: Leave materials and scripts hand-written everywhere. Rejected: they are precisely the structs developers extend most often; the accessor path keeps one source of truth.

---

### Newly reflected types (engine-wide review result)

**Decision**: Reflect exactly this set, chosen by the criteria "shown in editor, serialized, or useful to Python":

- `TagComponent::m_id` (adds `Guid` field support: read-only editor text, YAML scalar, Python string).
- `StaticMeshComponent::m_mesh`, `SkeletalMeshComponent::m_mesh` (asset refs).
- `ScriptComponent` script entries (module/class strings via custom accessor; runtime `ScriptData` instantiation logic unchanged).
- `Material::Variable` (+ its `Value` via accessor), `Material::m_variables`, `MaterialInstance::m_override_variables`, `MaterialInstance::m_material`; `DataType` enum.
- `Scene::EditorCameraData` (already serialized by hand; reflection dedupes `scene.cpp`).

Deliberately not reflected: `Entity`/`Scene` (already hand-bound in pybind), `GlobalSettings::PostProcessState`, `GlobalConstants`, `PerFrameConst`, render pipeline/framebuffer descriptions, `Animation`/`Skeleton` internals, mesh vertex/primitive storage, bakery types, editor-internal state.

**Rationale**: Criteria-driven and finite; each reflected type removes concrete hand-written code.

**Alternative considered**: Reflect everything reachable. Rejected: violates minimalism, exposes unstable internals to Python, and bloats codegen.

---

### Build-integrated codegen with a freshness gate

**Decision**: Add `python dev/z1.py gen-pybinds` (new `dev/commands/gen_pybinds.py`) that runs `utils/gen_pybinds.py` and reports `[OK]`/`[FAIL]` per the dev-scripts output contract. `generate` runs it after premake; `dcv --auto` runs it in check mode (regenerate to temp, diff against in-tree `py_engine.gen.cpp` and `engine/stubs/z1.pyi`, fail with exit 2 on drift). The generator is extended for the new field kinds (Guid → `str`, asset refs → path `str` property, `std::array`/containers of reflected types, custom-accessor fields via their exposed value type) and emits a generic `entity.add_component(name)` / `remove_component(name)` backed by the component hooks.

**Rationale**: Staleness is the current failure mode (`py_engine.gen.cpp` predates the particle system). A gate inside the existing CLI is the project's sanctioned automation path.

**Alternative considered**: VS prebuild event in premake. Rejected: bypasses the `z1.py` output contract, harder to gate in `dcv`, and the dev-scripts spec mandates the unified CLI.

---

### Fully reflection-driven editor

**Decision**: In `engine/editor/source/type_field.cpp` / `editor_layer.cpp`: render every reflected component through `show_type_fields` (delete the hand-written mesh/camera/script sections), generalize the asset drag-drop widget to any asset-trait field, add a read-only Guid widget and a script-entry list widget (backed by the custom accessor), build the add/remove-component menu by enumerating `TypeRegistry` for types with component hooks, and drive the material inspector from reflected `Material`/`MaterialInstance` variables.

**Rationale**: Requirement 4; the editor becomes a pure consumer of reflection metadata, so future reflected fields need zero editor code.

**Alternative considered**: Keep hand-written sections for "special" components. Rejected: those sections are what currently hide newly reflected fields from the inspector.

---

### Minimal Python input surface + demo game

**Decision**: Bind `InputSystem` polling (`z1.input.is_key_pressed(key)`) and the key/mouse constants in the manual module `py_engine.cpp` (small, hand-written, stable), keeping the existing event-listener API. The demo game lives in the game content root: `content/scene/demo_scene.yaml` (matches `--scene=demo_scene` already in `run_game.bat`) plus `content/scripts/*.py` (importable because `python_layer.cpp` appends the content root to `sys.path`). It uses only default engine assets (`engine/content/mesh/SM_*`, textures, materials) and exercises: component field get/set (`transform`, `light`, `camera`), `z1.globals`, `z1.scene.create_entity` / `add_component`, asset assignment by path, input polling, and event listeners.

**Rationale**: Requirement 5; also serves as an end-to-end smoke target for the whole reflection pipeline.

**Alternative considered**: Demo via event listeners only, no input polling. Rejected: continuous movement (WASD) via events is awkward and unrepresentative of real game scripts.

## Risks / Trade-offs

- **[Risk] YAML schema drift during serialization migration** — scene files on disk (`engine/content/scene/ParticleDemo.yaml`, test fixtures) must keep loading identically → **Mitigation**: key-derivation rules match current names, per-field adapters for exceptions, `test_scene_serialize` round-trip plus an explicit load-save-compare of the shipped scene as a validation task.
- **[Risk] Static-init registration in a static lib gets stripped by the linker** — new registrars in rarely-referenced TUs could vanish (the codebase already needs `ForceLinkPythonEngine`) → **Mitigation**: keep registrars in headers already pulled in by existing TUs; add a startup self-test (test exe asserts every expected type is registered).
- **[Risk] Custom accessors leak type-erasure bugs (lifetime, wrong type)** — get/set callbacks bypass the offset model → **Mitigation**: accessors are few, live next to their type, covered by unit tests; generic code paths stay unchanged when accessors are absent.
- **[Risk] Codegen gate false positives (line-ending/format churn)** — diff-based freshness could fail spuriously → **Mitigation**: generator output is deterministic (sorted, CRLF-normalized); check mode normalizes before diffing.
- **[Trade-off] Generic `entity.add_component("Name")` is stringly-typed** — typos fail at runtime, not import time → **Mitigation**: stubs keep typed per-component properties; the generic path raises a clear `KeyError` listing valid names.

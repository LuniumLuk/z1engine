## 1. Core Reflection Extensions

- [ ] 1.1 Add optional type-erased hooks to `TypeInfo` (`construct`, `add_to`, `remove_from`, `has_in`) and the asset-type trait (`is_asset_type<T>`) in `engine/runtime/source/core/reflection.h`; implement registration support in `engine/runtime/source/core/reflection.cpp`
- [ ] 1.2 Add the `REFLECTED_COMPONENT(Type)` macro (expands to `REFLECTED_STRUCT` + component hook registration) in `engine/runtime/source/core/core.h`, keeping existing macros untouched
- [ ] 1.3 Add `Guid` field support and optional custom get/set accessor callbacks to `FieldInfo` in `engine/runtime/source/core/reflection.h`
- [ ] 1.4 Add a registration self-test (all expected component types + `GlobalSettings` present in `TypeRegistry`) in `engine/test/` (new `test_reflection.cpp`); verify with `python dev/z1.py compile` and `python dev/z1.py test --filter test_reflection`

## 2. Reflect New Types

- [ ] 2.1 Reflect `TagComponent::m_id` (Guid, read-only + serializable) in `engine/runtime/source/scene/component/base.h`
- [ ] 2.2 Convert all component declarations to `REFLECTED_COMPONENT` and reflect `StaticMeshComponent::m_mesh` / `SkeletalMeshComponent::m_mesh` in `engine/runtime/source/scene/component/mesh.h`
- [ ] 2.3 Reflect `ScriptComponent` script entries (module/class string pairs via custom accessor) in `engine/runtime/source/scene/component/base.h`
- [ ] 2.4 Register the `DataType` enum via `REFLECT_ENUM` in `engine/runtime/source/render/data_types.h`; reflect `Material::Variable`, `Material::m_variables`, `MaterialInstance::m_material`, `MaterialInstance::m_override_variables` (with value accessor) in `engine/runtime/source/asset/material.h`
- [ ] 2.5 Reflect `Scene::EditorCameraData` in `engine/runtime/source/scene/scene.h`
- [ ] 2.6 Verify with `python dev/z1.py compile` and `python dev/z1.py test --filter test_reflection`

## 3. Reflection-Driven Serialization

- [ ] 3.1 Add generic reflection-driven YAML serialize/deserialize helpers honoring `FF_Serializable` (Guid, asset-ref-by-guid, enum, container, custom-accessor fields) in `engine/runtime/source/scene/scene.cpp` (or a new `engine/runtime/source/scene/serialization.{h,cpp}`), reusing `engine/runtime/source/util/yaml.h` converters
- [ ] 3.2 Migrate `Scene::load` / `Scene::save` in `engine/runtime/source/scene/scene.cpp`: replace per-component hand-written blocks, `global_settings`, and `editor_camera` with the generic path; keep YAML keys byte-compatible (per-field key adapters where hand-written names differ)
- [ ] 3.3 Migrate `Material::load`/`Material::save` and `MaterialInstance` persistence in `engine/runtime/source/asset/material.cpp` to the reflection-driven path
- [ ] 3.4 Verify round-trip: `python dev/z1.py compile`, then `python dev/z1.py test --filter test_scene_serialize`, plus load+save+diff of `engine/content/scene/ParticleDemo.yaml`

## 4. Python Binding Automation

- [ ] 4.1 Extend `utils/gen_pybinds.py`: Guid→`str`, asset-ref→path `str` property, enum fields, `std::array`/`std::vector` of reflected types, custom-accessor fields; emit generic `entity.add_component(name)` / `remove_component(name)` / `has_component(name)`; deterministic output (sorted, CRLF-normalized); add `--check` mode (regenerate to temp, diff, exit 2 on drift)
- [ ] 4.2 Add `dev/commands/gen_pybinds.py` implementing the `gen-pybinds` command per the dev-scripts output contract; register it in `dev/z1.py`; invoke it from `dev/commands/generate.py`; add the `gen-pybinds --check` step to `dev/commands/dcv.py` (auto-enabled when reflected headers change)
- [ ] 4.3 Regenerate `engine/runtime/source/python/py_engine.gen.cpp` and `engine/stubs/z1.pyi`; confirm particle types, `RenderMode`, and all newly reflected types are bound
- [ ] 4.4 Bind input polling (`z1.input.is_key_pressed`) and key/mouse constants in `engine/runtime/source/python/py_engine.cpp`; update stubs
- [ ] 4.5 Verify: `python dev/z1.py gen-pybinds`, `python dev/z1.py compile`, `python dev/z1.py test --filter test_pybind11`

## 5. Editor Upgrades

- [ ] 5.1 Generalize the asset drag-drop widget in `engine/editor/source/type_field.cpp` to any asset-trait field; add read-only Guid widget and script-entry list widget (custom-accessor backed)
- [ ] 5.2 Delete hand-written inspector sections for mesh/camera/script components in `engine/editor/source/type_field.cpp`; route all components through `show_type_fields`
- [ ] 5.3 Generate the add/remove-component menu from `TypeRegistry` component hooks in `engine/editor/source/editor_layer.cpp`
- [ ] 5.4 Drive the material inspector from reflected `Material`/`MaterialInstance` metadata (variable list, `DataType`-typed widgets, texture slots, override flags)
- [ ] 5.5 Verify: `python dev/z1.py compile` and `python dev/z1.py smoke`

## 6. Demo Game

- [ ] 6.1 Create game content root: `content/scene/demo_scene.yaml` (camera, sun light, ground cube, player entity with `ScriptComponent`) using default engine assets from `engine/content/mesh/`, `engine/content/texture/`, `engine/content/material/`
- [ ] 6.2 Write `content/scripts/player_controller.py` (WASD movement via `z1.input`, transform field writes) and `content/scripts/demo_world.py` (spawns entities via `z1.scene.create_entity` + `add_component`, assigns meshes by path, tweaks `z1.globals` sun settings, demonstrates `Script` lifecycle and event listener)
- [ ] 6.3 Verify: `run_game.bat` launches the demo, scripts load from the content root, no missing-asset errors in the log

## 7. Documentation & KB

- [ ] 7.1 Update `openspec/kb/ecs.md` reflection section (hooks, `REFLECTED_COMPONENT`, serialization is now reflection-driven) and `openspec/kb/python-scripting.md` (codegen command, generic component API, input API); update `openspec/kb/dev-scripts.md` with `gen-pybinds`
- [ ] 7.2 Add a short "Reflecting a new struct" how-to to `docs/CONTRIBUTING.md` (macro usage, flags, widget hints, codegen step)

## 8. Validation

- [ ] 8.1 Run `python dev/z1.py gen-pybinds` and confirm a clean working tree afterwards (deterministic output)
- [ ] 8.2 Run `python dev/z1.py compile` (Debug) with zero errors
- [ ] 8.3 Run `python dev/z1.py test` (all test executables, incl. `test_reflection`, `test_scene_serialize`, `test_pybind11`)
- [ ] 8.4 Run `python dev/z1.py smoke`
- [ ] 8.5 Run full gate: `python dev/z1.py dcv --auto` and parse the final `RESULT:` line (must be `"status": "ok"`)
- [ ] 8.6 Manually launch `run_game.bat` and confirm the demo game is playable (movement + spawned entities + sun changes visible)

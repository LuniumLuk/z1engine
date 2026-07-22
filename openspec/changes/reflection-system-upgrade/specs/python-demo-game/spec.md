## ADDED Requirements

### Requirement: Runnable demo game

A simple game implemented in Python SHALL ship under the game content root (`content/`), launchable via `run_game.bat` (`game.exe --game --scene=demo_scene`). It SHALL use only default engine assets (meshes, textures, materials from `engine/content/`).

#### Scenario: Launch the demo

- **WHEN** a developer runs `run_game.bat`
- **THEN** the engine loads `content/scene/demo_scene.yaml`
- **AND** the demo renders using default engine assets with no missing-asset errors

#### Scenario: Interactive behavior

- **WHEN** the player presses the movement keys
- **THEN** the player-controlled entity moves in the scene via a Python script's `on_update`
- **AND** the script logs or visibly reacts through reflected component fields

---

### Requirement: Demonstrated API surface

The demo scripts SHALL exercise the reflected Python APIs end to end.

#### Scenario: Covered APIs

- **WHEN** the demo scripts are inspected
- **THEN** they demonstrate, at minimum:
  - reading and writing reflected component fields (`transform`, `light`, `camera`)
  - `z1.globals` reflected settings
  - `z1.scene.create_entity` and generic `entity.add_component(name)`
  - assigning an asset to a reflected asset-reference field by path
  - `z1.input` polling and/or `register_event_listener`
  - the `Script` lifecycle (`on_attach`/`on_start`/`on_update`/`on_destroy`/`on_detach`)

#### Scenario: Scripts load from content root

- **WHEN** the demo scene attaches a script component referencing a module under `content/scripts/`
- **THEN** the module imports successfully at runtime

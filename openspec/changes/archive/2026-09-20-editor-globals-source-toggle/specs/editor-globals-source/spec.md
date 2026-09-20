## ADDED Requirements

### Requirement: Globals source toggle in the global settings panel

The editor's global settings panel SHALL provide a two-state source toggle labelled `editor` and `scene`, and
SHALL persist the selection as `globals_source` in `editor_settings.yaml` (integer; absent = `editor`). The
toggle SHALL only change where global settings are loaded from and saved to; the panel and the renderers SHALL
keep operating on the single live `GlobalSettings` object.

#### Scenario: Default source on a first run

- **WHEN** the editor starts without a `globals_source` entry in `editor_settings.yaml`
- **THEN** the toggle shows `editor` and the editor-globals rules apply

#### Scenario: Source survives a restart

- **WHEN** the user switches the toggle to `scene`, quits the editor and starts it again
- **THEN** the toggle shows `scene` and the scene-globals rules apply

#### Scenario: Switching source swaps the live globals

- **WHEN** the user switches the toggle from `editor` to `scene`
- **THEN** the current live globals are captured as the editor globals and the current scene's stored block is
  applied to the live globals on the same frame

#### Scenario: Switching back does not write the scene file

- **WHEN** the user switches the toggle from `scene` back to `editor`
- **THEN** the editor globals are re-applied to the live globals and the scene file is left unmodified

### Requirement: Editor globals storage

`EditorSettings` SHALL hold the editor globals as a reflected `GlobalSettings` record and persist it in
`editor_settings.yaml` under `global_settings`, using the same serialized key set as a scene's block. The
editor globals SHALL be applied exactly as-is (no preset overlay) and SHALL NOT be written into any scene
file while the source is `editor`.

#### Scenario: Startup restores the editor globals

- **WHEN** the editor starts in `editor` mode and `editor_settings.yaml` contains a `global_settings` block
- **THEN** that block is applied to the live globals after the initial scene has been loaded, replacing the
  values the scene contributed

#### Scenario: First run without stored editor globals

- **WHEN** the editor starts in `editor` mode and `editor_settings.yaml` has no `global_settings` block
- **THEN** the initial scene's stored values stay in effect and nothing is reported as an error

#### Scenario: Opening a scene keeps the editor globals

- **WHEN** the user opens another scene while the source is `editor`
- **THEN** the editor globals are re-applied after the scene is installed, so the live globals do not change

#### Scenario: Save Scene preserves the scene's block

- **WHEN** the user saves a loaded scene while the source is `editor`
- **THEN** the scene file keeps its previously stored `global_settings` block byte-for-byte (values
  unchanged), regardless of the live globals

#### Scenario: Quit persists the live globals

- **WHEN** the editor quits while the source is `editor`
- **THEN** `editor_settings.yaml` receives a `global_settings` block reflecting the live globals

### Requirement: Scene globals source

While the source is `scene`, global settings SHALL be loaded from the scene file and saved back into it: the
scene's stored block SHALL be applied to the live globals when the scene is loaded, and Save Scene SHALL flush
the live globals into that block before writing. `editor_settings.yaml`'s stored editor globals SHALL NOT be
updated while the source is `scene`.

#### Scenario: Opening a scene applies its block as-is

- **WHEN** the user opens a scene while the source is `scene`
- **THEN** the scene's stored global settings take effect exactly as authored, without preset or editor-globals
  interference

#### Scenario: Save Scene flushes the live globals

- **WHEN** the user changes globals in the panel and saves the scene while the source is `scene`
- **THEN** the scene file's `global_settings` block contains the live values

#### Scenario: Quit keeps the stored editor globals

- **WHEN** the editor quits while the source is `scene`
- **THEN** `editor_settings.yaml` retains its previously stored `global_settings` block (unsaved scene-mode
  edits are not written there)

### Requirement: Scene-owned globals block

`Scene` SHALL own its authored `global_settings` block: it SHALL cache the block when loading, capture the
live globals when creating a new scene, apply the cached block to the live globals on load, and write the
cached block on save. Scenes loaded without a block SHALL keep none until the editor flushes one in scene
mode.

#### Scenario: Load applies and caches the block

- **WHEN** a scene with a `global_settings` block is loaded
- **THEN** the block is applied to the live globals and retained on the scene for later saves

#### Scenario: Save writes the cached block

- **WHEN** a loaded scene is saved without a flush
- **THEN** the file contains the cached block (not a re-serialization of the live globals)

#### Scenario: New scenes carry a block

- **WHEN** a new scene is created in the editor and saved
- **THEN** the file contains a `global_settings` block captured from the live globals at creation time

#### Scenario: Scene without a block gains one only in scene mode

- **WHEN** a scene file has no `global_settings` block, the editor is in `scene` mode, and the user saves it
- **THEN** the saved file contains a block with the live globals

### Requirement: Game mode global settings authority

Game mode (`--game`) SHALL use only the scene's global settings: it SHALL NOT load `editor_settings.yaml`,
SHALL NOT apply editor globals, and SHALL NOT apply a quality preset.

#### Scenario: Game run ignores editor state

- **WHEN** the game is launched with `--game` while `editor_settings.yaml` holds editor globals and a
  `quality_preset`
- **THEN** rendering uses the loaded scene's stored `global_settings` values only

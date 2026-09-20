## REMOVED Requirements

### Requirement: Preset persistence

**Reason**: The stored preset is no longer applied automatically — it is selector state only. The old
"applied once at editor startup, after the initial scene load" behaviour contradicts the new globals-source
rules (`editor-globals-source`), where the startup globals come from the active source and nothing overlays a
preset.

**Migration**: The selection still persists as `quality_preset` in `editor_settings.yaml` and still applies
immediately when the user picks a preset in the panel (see "Preset application is manual" below). Settings
that must survive an editor restart belong in the `global_settings:` block of `editor_settings.yaml`
(editor mode) instead of relying on a preset being re-applied.

## ADDED Requirements

### Requirement: Preset application is manual

The editor settings file (`editor_settings.yaml`) SHALL persist the selected preset as `quality_preset`
(integer; absent = HIGH) as selector state only. The stored preset SHALL NOT be applied automatically — not at
editor startup, not on scene load, and not by any other load path. Presets SHALL mutate global settings only
when the user selects one in the global settings panel.

#### Scenario: Startup does not apply the stored preset

- **WHEN** the editor starts and `editor_settings.yaml` contains `quality_preset: 0` (LOW)
- **THEN** the global settings that take effect are exactly those loaded from the active globals source
  (`editor_settings.yaml` in `editor` mode, the scene file in `scene` mode), untouched by the preset

#### Scenario: Manual selection applies the preset

- **WHEN** the user selects a preset in the global settings panel
- **THEN** `apply_quality_preset` runs for that level and the corresponding global settings take effect on the
  next frame

#### Scenario: Selector reflects the stored value

- **WHEN** the editor starts and `editor_settings.yaml` contains `quality_preset: 1`
- **THEN** the panel's preset selector shows MEDIUM without changing any global setting

#### Scenario: Preset edits follow the globals source

- **WHEN** the user selects a preset while the globals source is `editor`
- **THEN** the changed values live in the editor globals (persisted to `editor_settings.yaml`) and reach a
  scene only after switching the source to `scene` and saving that scene

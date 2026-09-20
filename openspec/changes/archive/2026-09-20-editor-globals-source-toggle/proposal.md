## Why

The editor quality preset is applied into the live `GlobalSettings` object at startup (and the same object is
serialized into the scene on save), so opening a scene and saving it silently rewrites the scene's authored
TAA/bloom/AO/shadow settings even when the user never touched the preset. There is also no way to keep
editor-side render preferences (used while editing) separate from the settings the scene ships to the game,
and the editor forgets its own settings between sessions.

## What Changes

- Quality presets apply **only** when the user selects one in the global settings panel; the stored
  `quality_preset` is UI state persisted in `editor_settings.yaml` and is never re-applied automatically.
- The global settings panel gains an **editor / scene** source toggle (default: `editor`):
  - **editor** — globals load from and save to `editor_settings.yaml` only; the scene file's stored
    `global_settings` block is preserved verbatim on save.
  - **scene** — globals load from the scene file and are flushed back into it on Save Scene;
    `editor_settings.yaml`'s stored editor globals are left untouched.
- `Scene` owns its authored global settings block: cached at load (and captured at creation), applied to the
  live settings at load so game behavior is unchanged, and written back verbatim on save unless the editor is
  in scene mode (then the live globals are flushed into it first).
- Switching the toggle swaps the live globals in place: editor → scene captures the editor globals and applies
  the current scene's block; scene → editor re-applies the editor globals without writing the scene file.
- Game mode (`--game`) keeps using only the scene's globals: no preset and no `editor_settings.yaml` state is
  involved.
- No change to the preset contents (LOW/MEDIUM/HIGH table).

## Capabilities

### New Capabilities
- `editor-globals-source`: editor/scene globals source toggle, editor globals persistence in
  `editor_settings.yaml`, scene globals block preservation/flush, and game-mode authority over globals.

### Modified Capabilities
- `graphics-quality-presets`: the stored preset is no longer applied at editor startup; presets apply only on
  manual selection in the panel.

## Impact

- `engine/runtime/source/scene/scene.{h,cpp}` — scene caches its authored `global_settings` block; new
  `apply_global_settings()` / `capture_global_settings()` helpers; `save()` writes the cached block.
- `engine/editor/source/editor_layer.{h,cpp}` — `EditorSettings` gains the editor globals snapshot and the
  globals source; panel toggle; startup / scene-load / Save Scene / quit wiring; startup preset application
  removed.
- `editor_settings.yaml` (local, gitignored) gains `globals_source` and a `global_settings:` block.
- Knowledge base `openspec/kb/perf-probing-and-quality.md` and the capture instructions in
  `docs/PROFILING_MACOS_OPENGL.md` (the preset no longer applies at startup).

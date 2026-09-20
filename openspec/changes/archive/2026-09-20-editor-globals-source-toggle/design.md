## Context

`GlobalSettings` is a single live object (`g_runtime_context.m_global`) shared by the renderers, the editor's
settings panel and the scene serializer:

- `Scene::load` deserializes the scene's `global_settings` block into the live object.
- `Scene::save` serializes the live object back into the scene.
- `EditorLayer`'s constructor calls `apply_quality_preset(...)` after the initial scene load and the panel's
  preset combo calls it again on selection, mutating the live object.

Because the live object is the save source, a preset applied at startup is silently baked into the scene on
the next Save Scene, and the editor has no persistent settings of its own (only `quality_preset` is stored in
`editor_settings.yaml`). Game mode (`--game`, `GameLayer`) never constructs `EditorLayer`, so it already uses
only the scene's globals and never sees a preset — that authority must be preserved.

## Goals / Non-Goals

**Goals:**
- Presets mutate settings only on an explicit user selection in the panel.
- Editor render preferences persist across editor sessions (`editor_settings.yaml`) without touching scenes.
- An explicit editor/scene source toggle decides where globals load from and save to; scenes never lose their
  authored block by accident.
- Keep a single live `GlobalSettings` object for rendering/UI (no parallel UBO).

**Non-Goals:**
- Changing preset contents or the preset UI elsewhere.
- Making presets available to game builds or to scripts.
- Reworking how `GlobalSettings` is reflected/serialized or how scenes store other data.
- Automatic scene-file watching/reloading.

## Decisions

**1. `Scene` owns its authored `global_settings` block (cached `YAML::Node`).**
`Scene::load` caches the block and keeps applying it to the live settings (game behavior unchanged);
`Scene::create` captures the live settings so new scenes still ship a block; `Scene::save` writes the cached
block instead of the live object. Two helpers expose the flow for the editor: `apply_global_settings()`
(cached → live) and `capture_global_settings()` (live → cached).
*Alternatives:* (a) a `Scene::save(bool write_globals)` flag — rejected, because in editor mode the scene
would then be written *without* its block, silently changing the game's settings; (b) caching a second
`GlobalSettings` inside `Scene` — rejected: the type owns a `UniformBuffer` (`shared_ptr`, created in its
constructor), so copies alias the same GPU buffer and a second instance allocates an unused one; (c) an
editor-side per-scene snapshot — rejected: the data belongs to the scene and the runtime already owns the
serializer side.

**2. Editor globals are stored in `EditorSettings` as a YAML node.**
`EditorSettings` gains `YAML::Node globals` (the editor globals, same reflected key set as a scene block) plus
`GlobalsSource globals_source {Editor}`. `save()` writes `global_settings:` from the live object when the
source is `editor`, or re-emits the cached node otherwise; `load()` only caches the node (applying is
deferred). Reflection-driven `serialize_type`/`deserialize_type` are reused, so no new format is invented.
*Alternative:* a second `GlobalSettings` member — rejected for the same UBO/aliasing reason as above.

**3. One live object; the toggle swaps its contents.**
The panel edits (and the renderers read) the live `GlobalSettings` exactly as today. The source only decides
which storage is loaded into it and which storage receives it:
- startup (after the initial scene load): `editor` → apply the stored editor globals (exact, no preset);
- scene load: `editor` → re-apply the editor globals after the scene is installed; `scene` → leave the
  scene's values applied by `Scene::load`;
- Save Scene: `scene` → `Scene::capture_global_settings()` before `save()`; `editor` → plain `save()`
  (cached block preserved verbatim);
- quit: `editor` → capture the live object into the node and write `editor_settings.yaml`; `scene` → write the
  unchanged node;
- toggle: editor → scene captures the live object into the node and applies the current scene's block;
  scene → editor re-applies the node (no scene write).
Loading and saving are therefore always "exact as-is": nothing is post-processed by a preset.

**4. Presets stay editor-only and manual.**
`apply_quality_preset` is called from exactly one place: the preset combo's selection handler. The stored
`quality_preset` remains UI state so the combo shows the last selection.

**5. Layers are destroyed before the engine services they use (`RuntimeContext::shutdown`).**
Persisting the editor globals needs the live `GlobalSettings` at shutdown, but `shutdown()` reset
`m_global` before `m_layer_stack`, so a destructor-based capture would read a destroyed object. The layer
stack is now reset first, which also lets `EditorLayer::on_detach()` (the designed "layer is going away"
hook) persist the settings once; `persist_settings()` additionally runs from the destructor for teardown
paths outside the stack and guards against a missing engine context.
*Alternative:* capturing the live globals every frame — rejected (per-frame serialization for a value that
only changes on user input).

## Risks / Trade-offs

- [Automation that selected behaviour via `quality_preset` in `editor_settings.yaml` stops working] →
  captures must now set the `global_settings:` block (editor mode) or switch to scene mode; the KB page and
  `docs/PROFILING_MACOS_OPENGL.md` capture notes are updated in this change.
- [A user in `editor` mode no longer sees the scene's authored settings in the panel] → intended: the toggle
  exists exactly for that; switching to `scene` applies the scene's block immediately.
- [Leaving scene mode without Save Scene discards those unsaved edits (the live object is replaced by the
  editor globals)] → documented; Save Scene is the commit point in scene mode.
- [Old `editor_settings.yaml` files] → missing `globals_source` defaults to `editor`, and a missing
  `global_settings` block means the scene's values stay in effect at startup (same as today minus the preset).
- [Scene files without a block] → loaded scenes keep no block (nothing is written in editor mode); in scene
  mode the block is created on the next Save Scene.

## Migration Plan

No scene-format change; both files' new keys are additive and optional. Rollback is deleting the new
`global_settings`/`globals_source` keys from `editor_settings.yaml`.

## Open Questions

None.

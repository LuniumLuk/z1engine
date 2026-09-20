## 1. Runtime: scene owns its globals block

- [x] 1.1 `scene.h`: add `YAML::Node m_global_settings` plus
      `apply_global_settings()` / `capture_global_settings()` declarations (east-const style, tabs)
- [x] 1.2 `scene.cpp`: `load()` caches the `global_settings` node before applying it to the live settings
- [x] 1.3 `scene.cpp`: `create()` captures the live globals so new scenes ship a block
- [x] 1.4 `scene.cpp`: `save()` emits the cached block instead of re-serializing the live globals
- [x] 1.5 Compile check (`python dev/z1.py compile`, 0 errors)

## 2. Editor: editor globals, source toggle, wiring

- [x] 2.1 `editor_layer.h`: add `GlobalsSource { Editor, Scene }` and the `EditorSettings` members
      (`globals_source`, `YAML::Node globals`) with `apply`/`capture` helpers
- [x] 2.2 `editor_layer.cpp`: `EditorSettings::save()` writes `globals_source` and the `global_settings`
      block (live globals in `editor` mode, the cached node otherwise); `load()` reads both
- [x] 2.3 `editor_layer.cpp`: replace the startup `apply_quality_preset` call with an editor-globals apply in
      `editor` mode (after the initial scene load)
- [x] 2.4 `editor_layer.cpp`: `load_scene()` re-applies the editor globals in `editor` mode
- [x] 2.5 `editor_layer.cpp`: Save Scene menu item flushes the live globals into the scene in `scene` mode
- [x] 2.6 `editor_layer.cpp`: `~EditorLayer` captures the live globals in `editor` mode before saving settings
- [x] 2.7 `editor_layer.cpp`: add the editor/scene toggle to the settings panel and implement the switch
      (editor → scene: capture + apply the scene's block; scene → editor: apply the node)
- [x] 2.8 `RuntimeContext::shutdown()` resets the layer stack before the engine services it uses, so
      `on_detach` can persist the settings while the context is alive
- [x] 2.9 Compile check (`python dev/z1.py compile`, 0 errors)

## 3. Verification

- [x] 3.1 `python dev/z1.py format --dry-run` clean (only the pre-existing
      `engine/content/scripts/kinematic_platform.py` drift), then `compile` (0 errors)
- [x] 3.2 `python dev/z1.py smoke --frames 10` ok (3.7s), 0 GL errors; the run leaves `content/helmet.yaml`
      byte-identical
- [x] 3.3 Editor-mode captures: the first run (no stored block) renders the scene's authored values and
      captures them at quit; `quality_preset: 0` vs `1` produce byte-identical screenshots (md5
      `cbb13c08...`), so the stored preset is no longer applied; an editor block with `pp_exposure: 3`
      renders brighter (99.99% pixels differ, mean delta 32), proving the editor globals drive rendering
- [x] 3.4 Scene-mode captures: with the editor block still at `pp_exposure: 3` and `globals_source: 1` the
      screenshot is byte-identical to the baseline (editor globals ignored); raising the scene's own
      `pp_exposure` to 3 produces the same image as the editor-mode exposure-3 capture, proving the scene's
      block is applied as-is
- [x] 3.5 Round-trip check (temporary headless Save Scene hook, removed afterwards): editor mode keeps the
      scene's `global_settings` block unchanged (exposure 3.5 live value not flushed); scene mode flushes it
      (`pp_exposure: 3.5` written); quitting in scene mode keeps the stored editor block; `editor_settings.yaml`
      gains `globals_source` + `global_settings`
- [x] 3.6 Game-mode check: `editor_settings`/`apply_quality_preset` appear only under `engine/editor/source`
      and the editor layer is constructed only by `EditorApp`, so `--game` cannot read editor globals or
      presets (a `--game` run of the editor-authored helmet scene still aborts on the pre-existing "no main
      camera" limitation, unrelated to this change)

## 4. Documentation

- [x] 4.1 Update `openspec/kb/perf-probing-and-quality.md` (source toggle, manual-only presets, capture
      recipe)
- [x] 4.2 Update the capture instructions in `docs/PROFILING_MACOS_OPENGL.md`
- [x] 4.3 Archive the change under `openspec/changes/archive/` once verification passes

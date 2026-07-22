# Python Scripting
> Summary: Python 3.14 gameplay scripting integration via pybind11
> Scope: engine/runtime/source/python/, content/scripts/, 3rdparty/python314/

## Architecture

| Component | File | Role |
|-----------|------|------|
| `PythonLayer` | `python/python_layer.cpp` | VM lifecycle, interpreter init, `PYTHONHOME` setup |
| `z1` module | `python/py_engine.cpp` | Built-in module: `Entity`, `Transform`, `Vec3`, logging |
| `PythonScript` | `python/python_script.cpp` | C++ wrapper implementing `ScriptBase` interface |

## Script Lifecycle

- `on_attach()` -- called when script is bound to entity
- `on_update(dt)` -- called each frame with delta time
- `on_detach()` -- called when script is removed

## Writing a Script

- Location: `content/scripts/`
- Must define a class inheriting from `z1.Script`

```python
import z1

class MyScript(z1.Script):
    def on_attach(self):
        z1.log_info("attached")

    def on_update(self, dt):
        # Input polling
        if z1.input.is_key_pressed(z1.input.KEY_W):
            t = self.entity.transform
            loc = t.location
            loc.y += 1.0 * dt
            t.location = loc

    def on_detach(self):
        z1.log_info("detached")
```

## Input API

- `z1.input.is_key_pressed(keycode)` — poll keyboard state (GLFW key codes)
- `z1.input.is_mouse_button_pressed(button)` — poll mouse button state
- `z1.input.get_mouse_pos()` — returns `(x, y)` tuple of cursor position
- Key constants on `z1.input`: `KEY_W`, `KEY_A`, `KEY_SPACE`, `KEY_ESCAPE`, `KEY_LEFT_SHIFT`, `KEY_LEFT_CONTROL`, etc.
- Mouse constants: `MOUSE_BUTTON_LEFT`, `MOUSE_BUTTON_RIGHT`, `MOUSE_BUTTON_MIDDLE`
- Cursor control: `z1.hide_cursor()`, `z1.show_cursor()`, `z1.center_cursor()`, `z1.is_cursor_hidden()`

## Code Generation

- `python dev/z1.py gen-pybinds` — regenerates `py_engine.gen.cpp` and `z1.pyi` from reflected C++ types
- `python dev/z1.py gen-pybinds --check` — dry-run check for stale bindings (exit 2 if drift)
- Run after adding/removing `REFLECTED_FIELD`, `REFLECTED_COMPONENT`, or `REFLECT_ENUM`
- Generated bindings include per-type entity properties (e.g., `entity.light`, `entity.camera`)
- `ScriptComponent` is bound manually as `z1.Script` (excluded from auto-generation)

## Entity Component API

- `entity.light` — access `LightComponent` (read-only, returns None if absent)
- `entity.camera`, `entity.transform`, `entity.tag`, `entity.skeletal_mesh`, `entity.static_mesh`
- `entity.sky_light`, `entity.sprite`, `entity.animation`, `entity.particle`, `entity.postprocess_volume`
- `entity.add_static_mesh(path)`, `entity.add_skeletal_mesh(path)`, `entity.add_camera()`
- `entity.add_script(module, class_name)` — attach a Python script
- `entity.is_valid()` — check if entity is still alive

## Attaching Scripts (C++)

```cpp
entity->attach_script<PythonScript>("my_script_module", "MyScriptClass");
```

## Serialization

- `ScriptComponent` stores module + class name
- Scene serializer saves/loads to YAML
- Editor inspector lists available scripts
- Class name auto-inferred from file: `test_mover.py` -> `TestMover`

## Build Dependencies

- `python314.dll` must be in executable directory
- `python314.zip` (stdlib) in `pyenv/`
- Files at: `3rdparty/python314/`

## Asset Integration

- `AssetManager` scans `.py` files in `content/`
- Registers as `ScriptAsset`
- Editor `ScriptComponent` inspector shows available scripts

-> see [ecs.md#scriptcomponent]

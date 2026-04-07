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
        t = self.entity.transform
        loc = t.location
        loc.y += 1.0 * dt
        t.location = loc

    def on_detach(self):
        z1.log_info("detached")
```

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

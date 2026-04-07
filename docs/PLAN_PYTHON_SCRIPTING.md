> **DEPRECATED** — This document has been migrated to `openspec/kb/python-scripting.md`. This file is kept for reference only.

# Plan: Python Scripting for Gameplay

This document outlines the implementation of Python scripting support for the z1engine, enabling gameplay logic to be written in Python 3.14.

## 1. Overview
The goal is to allow developers to attach Python scripts to entities in the scene. These scripts receive lifecycle events (`on_attach`, `on_update`, `on_detach`) and can manipulate the entity's transform and other components.

## 2. Architecture
The system consists of the following components:

-   **PythonLayer (`engine/runtime/source/python/python_layer.cpp`)**:
    -   Manages the Python Virtual Machine (VM) lifecycle.
    -   Initializes the interpreter with isolated configuration.
    -   Sets up `PYTHONHOME` to point to the embedded `pyenv` directory.

-   **z1 Module (`engine/runtime/source/python/py_engine.cpp`)**:
    -   A built-in Python module exposing engine types (`Entity`, `Transform`, `Vec3`).
    -   Provides logging utilities (`log_info`, `log_warn`, `log_error`).

-   **PythonScript Wrapper (`engine/runtime/source/python/python_script.cpp`)**:
    -   A C++ class that wraps a Python object instance.
    -   Implements the `ScriptBase` interface to forward engine events to Python.
    -   Handles instantiation of Python classes based on module and class names.

-   **Serialization (`engine/runtime/source/scene/scene.cpp`)**:
    -   `ScriptComponent` stores the module and class name.
    -   The scene serializer saves and loads these names to/from YAML.

## 3. Usage Guidelines

### Writing a Script
Scripts should be placed in `content/scripts/`. A script must define a class that inherits from `z1.Script`.

```python
import z1

class MyScript(z1.Script):
    def on_attach(self):
        z1.log_info("Script attached!")
        # self.entity is automatically injected

    def on_update(self, dt):
        # Modify transform
        t = self.entity.transform
        loc = t.location
        loc.y += 1.0 * dt
        t.location = loc

    def on_detach(self):
        z1.log_info("Script detached!")
```

### Attaching a Script
In C++:
```cpp
entity->attach_script<PythonScript>("my_script_module", "MyScriptClass");
```

## 4. Build & Environment
-   **Dependencies**: The engine requires a Python 3.14 environment.
    -   `python314.dll` must be in the executable directory.
    -   `python314.zip` (standard library) or a `Lib` folder must be present in `pyenv`.
-   **Setup**:
    -   Ensure `pyenv/Lib` contains the standard library (extracted from zip if necessary).

## 5. Testing
-   **Verification**: Run `engine\bin\test\Release\test_scene_serialize.exe`.
    -   It loads `content/scripts/test_mover.py`.
    -   It attaches `TestMover` to an entity.
    -   It verifies that the entity moves over time.

## 6. Implementation Status
-   [x] Python VM Initialization (Fixed `encodings` module issue).
-   [x] `z1` Module Bindings (`Entity`, `Transform`, `Vec3`).
-   [x] `PythonScript` C++ Wrapper.
-   [x] Script Component Serialization.
-   [x] Integration Test (`test_mover.py`).
-   [x] Asset Manager Integration:
    -   Scans `.py` files in `content/`.
    -   Registers them as `ScriptAsset`.
-   [x] Editor Integration:
    -   `ScriptComponent` inspector lists available scripts.
    -   Automatically infers class name from file name (e.g., `test_mover.py` -> `TestMover`).

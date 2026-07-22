## ADDED Requirements

### Requirement: Code generation is part of the dev CLI

Python binding generation SHALL be invocable via `python dev/z1.py gen-pybinds`, following the dev-scripts output contract (`[OK]`/`[FAIL]` lines, `RESULT:` summary JSON, normalized exit codes). It SHALL regenerate `engine/runtime/source/python/py_engine.gen.cpp` and `engine/stubs/z1.pyi` from the current reflected declarations.

#### Scenario: Manual regeneration

- **WHEN** a developer runs `python dev/z1.py gen-pybinds`
- **THEN** both generated files are rewritten deterministically from the current headers
- **AND** the command reports `[OK]` with a `RESULT:` summary line

#### Scenario: Integrated into generate

- **WHEN** `python dev/z1.py generate` runs
- **THEN** binding generation runs after premake project generation
- **AND** a generation failure fails the command with a non-zero exit code

---

### Requirement: Freshness gate

`python dev/z1.py dcv --auto` SHALL verify that checked-in generated bindings match what the generator produces from the current headers.

#### Scenario: Stale bindings detected

- **WHEN** a reflected declaration changed without regenerating bindings
- **THEN** the gate fails with exit code 2 and a message naming the stale file(s)

#### Scenario: Clean bindings pass

- **WHEN** generated files are up to date
- **THEN** the gate reports `[OK]` and adds no diff noise to the working tree

---

### Requirement: Bindings cover all reflected types

The generated module SHALL bind every reflected struct and enum: `py::enum_` for each `REFLECT_ENUM` type, `py::class_` with `def_readwrite` for each reflected field, typed component properties on `Entity`, and matching type annotations in `engine/stubs/z1.pyi`.

#### Scenario: New field kinds map to Python types

- **WHEN** the generator encounters a `Guid` field, an asset-reference field, an enum field, a `std::array`/`std::vector` field, or a custom-accessor field
- **THEN** it emits a Python property of the corresponding type (`str` for Guid and asset paths, the enum class, `list`, the accessor's value type)
- **AND** the stub file annotates the same types

#### Scenario: Previously missing bindings appear

- **WHEN** bindings are regenerated
- **THEN** `ParticleComponent`, `RenderMode`, `ParticleBlendMode`, `EmitterShape`, and all newly reflected types from this upgrade are importable from the `z1` module

---

### Requirement: Generic component management from Python

The `Entity` binding SHALL expose generic component management backed by registry hooks: `add_component(name)`, `remove_component(name)`, and `has_component(name)`, in addition to the existing typed properties.

#### Scenario: Add component by name

- **WHEN** a script calls `entity.add_component("Light")`
- **THEN** a `LightComponent` with default values is attached and readable via `entity.light`
- **AND** an unknown name raises a `KeyError` listing valid component names

---

### Requirement: Python input surface

The `z1` module SHALL expose keyboard/mouse input polling (`z1.input.is_key_pressed`) and the key/mouse button constants, alongside the existing event-listener API.

#### Scenario: Poll a key in on_update

- **WHEN** a Python script calls `z1.input.is_key_pressed(z1.input.Key.W)` inside `on_update`
- **THEN** it returns `True` while the W key is held

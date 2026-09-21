# guid-system Specification

## Purpose
TBD - created by archiving change optimize-guid-system. Update Purpose after archive.
## Requirements
### Requirement: Guid is a fixed 128-bit value type

`z1::Guid` SHALL be a 16-byte value type holding four 32-bit words (`m_data[4]`), with `m_data[0]` storing the
first 8 hex digits. It MUST NOT allocate or hold text, MUST support copying, comparison by value and use as an
unordered-map key, and MUST NOT provide random generation.

#### Scenario: Default guid is invalid

- **WHEN** a `Guid` is default-constructed
- **THEN** all four words are zero and `is_valid()` returns false

#### Scenario: Any non-zero word makes the guid valid

- **WHEN** a `Guid` has at least one non-zero word
- **THEN** `is_valid()` returns true and two guids are equal iff all four words match

### Requirement: Engine assets derive ids from their path with a zero prefix

For assets in the `engine` root, the guid SHALL be derived from the asset path alone: `m_data[0]` MUST be zero
(the first 8 hex digits are all zero) and the remaining 96 bits MUST be a deterministic hash of the path.

#### Scenario: Engine path yields a prefixed id

- **WHEN** the engine asset `mesh/SM_Cube` is derived
- **THEN** the resulting guid's first 8 hex digits are `00000000` and the id is identical on every run

#### Scenario: Distinct engine paths differ

- **WHEN** two different engine paths are derived
- **THEN** the two guids differ

### Requirement: Other assets derive ids from root and path and never carry the engine prefix

For assets outside the `engine` root, the guid SHALL be derived from `root + "/" + path`. If the derived first 32
bits would be zero they MUST be flipped (bit 63 set) so the first 8 hex digits are never all zero.

#### Scenario: Root is part of the identity

- **WHEN** the same relative path exists under two different roots
- **THEN** the two guids differ

#### Scenario: Foreign ids are disjoint from engine ids

- **WHEN** any non-engine asset id is derived
- **THEN** its first 8 hex digits are not `00000000`

### Requirement: Guid text form is 32 lowercase hex digits

`Guid::to_string()` SHALL produce exactly 32 lowercase hex digits with no separators, and parsing SHALL accept the
same form (dashes tolerated). All-zero text means invalid. Text that is not hex MUST parse to an invalid guid
instead of raising an error.

#### Scenario: Round-trip

- **WHEN** a guid is converted to text and parsed back
- **THEN** the parsed guid equals the original

#### Scenario: Path strings are not guids

- **WHEN** the string `mesh/SM_Cube` is parsed as a guid
- **THEN** the result is an invalid guid (no exception, no partial parse)

### Requirement: Asset scan derives ids and ignores stored values

`AssetManager::scan_content()` SHALL compute `meta.guid` from the root and path for every discovered asset (yaml
assets, python scripts, shaders) and MUST NOT trust the stored yaml value. An unparseable stored guid MUST NOT
abort scanning, and duplicate derived ids MUST still be reported.

#### Scenario: Stale stored guid is overridden

- **WHEN** a yaml asset stores an old random uuid in `meta.guid`
- **THEN** the registered id is the derived value for that root and path

#### Scenario: Unparseable stored guid keeps the asset registered

- **WHEN** a yaml asset stores a non-hex guid string
- **THEN** scanning completes and the asset registers under its derived id

### Requirement: References resolve by path or by derived id

`AssetManager::resolve_guid()` SHALL accept legacy path strings (`mesh/SM_Cube`, `shader/pbr.glsl`) and 32-hex
guids. Reflected plain-`Guid` fields SHALL deserialize through the same resolver so scenes and materials written
with path strings keep loading. Saving SHALL write the 32-hex form.

#### Scenario: Path reference still loads

- **WHEN** a scene or material references `mesh/SM_Cube` or a texture by path
- **THEN** the reference resolves to the same asset as the derived id

#### Scenario: Saved references are hex

- **WHEN** a scene, material or material instance is saved
- **THEN** asset references are written as 32-hex guids

### Requirement: Importers and creators derive ids instead of generating them

Importers (`TextureImporter`, `ObjImporter`, `GltfImporter`) and runtime creators SHALL derive the asset id from the
resolved root and path of the asset being written (`Material::create`, `MaterialInstance::create`,
`Mesh::Storage::import`). No code path SHALL mint a random or arbitrary id.

#### Scenario: Import derives the id

- **WHEN** a texture is imported to `sandbox-tests/test_import/texture/T_awesomeface`
- **THEN** the saved `meta.guid` equals the derived id for that root and path

#### Scenario: No random id API remains

- **WHEN** the codebase is searched for random guid generation
- **THEN** no `Guid::generate`-style API exists and no call site fabricates ids

### Requirement: Migration tool rewrites stored guids and references

A one-shot Python tool SHALL migrate existing content: pass 1 builds an `old -> new` map from the stored guid of
every yaml asset in every root, pass 2 rewrites each `meta.guid` to the derived value and every reference string
equal to an old id to the corresponding new id. The tool MUST be idempotent and MUST NOT rewrite path-style
references.

#### Scenario: First run rewrites

- **WHEN** the tool runs on content containing uuids
- **THEN** stored guids become 32-hex derived ids and uuid references are remapped

#### Scenario: Second run is a no-op

- **WHEN** the tool runs again on the migrated content
- **THEN** no file changes

### Requirement: Python tooling mirrors the id derivation

A shared Python helper SHALL compute ids bit-identically to the engine so tooling (migration, content generator)
can write valid ids. C++ and Python SHALL agree on recorded test vectors.

#### Scenario: Cross-language vectors match

- **WHEN** the recorded test vectors are evaluated in C++ and in Python
- **THEN** both produce the same 32-hex ids, including the engine zero prefix and the foreign flip rule


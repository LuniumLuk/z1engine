# Design: deterministic 128-bit path-derived guids

## Context

- `z1::Guid` is `struct Guid { std::string value; }`. Random ids are minted by `Guid::generate()` at import time
  (`TextureImporter`, `Mesh::Storage::import`, `Material::create`, `MaterialInstance::create`) and persisted.
- Asset scan (`AssetManager::scan_content`) reads `meta.guid` straight from each yaml; scripts and shaders get
  path-string ids (`Guid::make(path)`). So the id space mixes random UUID strings and path strings.
- References exist in three flavours: path strings (`mesh/SM_Cube`, `shader/pbr.glsl`, `texture/T_white`), random
  UUIDs (material instance texture overrides, prefab `meta.guid`), and the reflection serializer's plain-`Guid`
  fields (deserialize currently assigns the raw yaml string to `guid->value`).
- Assets live in roots declared in `core/io.cpp` (`{"engine", "engine/content", 10}` plus the default root and user
  roots); `AssetMeta.root` names the root, `AssetMeta.path` is root-relative.
- Constraints: C++17 on MSVC x64 (no `unsigned __int128`), yaml-cpp for serialization, Python tooling must be able
  to compute the same ids (migration + content generator), ~200 yaml assets in this repo.

## Goals / Non-Goals

**Goals:**

- Ids are a pure function of identity: engine assets of their path, other assets of `root + "/" + path`.
- Engine ids carry a zero 32-bit prefix; user ids can never carry it (disjoint spaces, cheap "is engine" test).
- `Guid` is a 16-byte value type (4x `uint32_t`): no heap, cheap compare/hash, memcpy-safe.
- Existing content keeps working: path references still resolve, and a scripted migration converts stored guids.
- Python and C++ hash implementations are bit-identical and covered by shared test vectors.

**Non-Goals:**

- No content-addressed (hash-of-bytes) ids, no id versioning/alias layer, no change to the asset file layout other
  than the guid values inside yaml.
- No editor UX redesign; the browser keeps showing the id text with the new format.
- No support for hand-written ids in yaml: anything not derivable from root + path is ignored.

## Decisions

**D1 -- Hash: two FNV-1a-64 passes over domain-prefixed keys (128-bit total).**
`high = fnv1a64("z1a:" + key)`, `low = fnv1a64("z1b:" + key)`, `Guid = { high, low }`.
Alternatives: single FNV-1a-128 (needs a 128-bit multiply; MSVC has no `unsigned __int128`, so it must be emulated
with 2-limb arithmetic), xxHash128 (new third-party dependency), MD5/SHA (dependency + heavier). Two independent
64-bit passes need no new dependency, are trivial to mirror in Python, and 128 output bits with independent halves
is far beyond what a few hundred paths require.

**D2 -- Key derivation and id spaces.**
- Engine root (`root == "engine"`): `key = path`, then `m_data[0] = 0` (the first 8 hex digits), the 96 remaining
  bits come from the hash.
- Any other root: `key = root + "/" + path` (default root contributes an empty name, e.g. `"/MI_HappyFace"`);
  if `m_data[0]` would be zero, bit 63 of `high` is set so the first 8 digits are never all zero.
Rationale: engine content is engine-owned and stable; user content can never be confused with it, and the
`m_data[0] == 0` test replaces today's "is it in the engine root" bookkeeping.

**D3 -- Struct and text format.**
`struct Guid { uint32_t m_data[4]; }` with `m_data[0]` = the first 8 hex digits (textual order == array order).
Text form: 32 lowercase hex digits, no separators; all-zero means invalid/empty. `is_valid()` = any word non-zero.
Alternatives: 2x `uint64_t` (works, but the "first 8 digits" rule is stated in words, so words are used);
`std::array` (same semantics, raw array keeps it a POD that is exactly 16 bytes).

**D4 -- The scan derives and does not trust.**
`scan_content()` computes `meta.guid = Guid::from_root_and_path(root, path)` for yaml assets, scripts and shaders,
overriding whatever the yaml stored. `convert<Guid>::decode` is lenient: 32 hex digits (dashes tolerated) parse,
anything else yields an invalid id instead of throwing. Rationale: old projects keep loading before migration, a
stale or hand-edited guid can never desync identity, and the "guid" field in yaml becomes documentation.

**D5 -- Reference resolution.**
`AssetManager::resolve_guid(string)` keeps its order: path lookup first (so `mesh/SM_Cube` and `shader/pbr.glsl`
still resolve), then hex parse for direct ids. Reflected plain-`Guid` fields deserialize through the same resolver,
so scenes written with path strings keep loading. Saving always writes hex ids.

**D6 -- No random ids.**
`Guid::generate()` is deleted. Importers and `create()` paths resolve the target root/path first and derive the id;
this removes the "id exists before the file does" state that required a registry to keep in sync.

**D7 -- One Python hash mirror, two consumers.**
`utils/z1_guid.py` implements the hash and text codec identically to C++; the migration script and the content
generator import it. The C++ test and a Python self-check assert the same vectors, so drift is caught in CI-style
test runs rather than by a broken reference.

**D8 -- Bindings and tooling.**
`py_engine.cpp` registers `py::class_<Guid>` (`to_string`, `from_string`, `__repr__`, `__eq__`) so the generated
`Material.shader_guid` property keeps working. `engine/tool/assetkit/assetkit_core.py` mirrors the derivation so its
validation matches the engine ("duplicate guid" becomes structurally impossible; it now reports *unresolvable*
references instead).

## Risks / Trade-offs

- **Stale ids in content outside the migration** → migration tool maps old→new via the files themselves; path
  references never needed migration at all; anything left over fails loudly as an unresolved reference.
- **Hash collisions** → 96-bit engine / 128-bit user space; the duplicate-id registry check is kept, so a collision
  is reported at scan time instead of silently aliasing.
- **C++/Python hash drift** → shared test vectors in the C++ test plus a Python assertion.
- **MSVC lacks 128-bit integers** → avoided by construction (two 64-bit passes).
- **Migration touches ~170 files** → the script is idempotent and mechanical; run over git so the diff is reviewable.
- **Mesh `.bin` files embed a yaml copy with the old guid** → the loader ignores the embedded meta; regenerating
  content (generator tool) writes the new format, Suzanne keeps a stale embedded value that is never read.

## Migration Plan

1. Land `Guid`, scan, resolver and serialization changes.
2. Run `utils/migrate_guids.py` over all roots: pass 1 builds `old -> new` from every yaml's stored guid, pass 2
   rewrites `meta.guid` and any reference string equal to an old id.
3. Regenerate engine content with `utils/gen_content.py` (new-format ids, optimized geometry).
4. Rollback: revert the commit; yaml changes are mechanical and the old ids remain valid for the old code.

## Open Questions

- Whether yaml should eventually stop writing `meta.guid` (kept for now: readable diffs, and `AssetMeta` already
  serializes it).

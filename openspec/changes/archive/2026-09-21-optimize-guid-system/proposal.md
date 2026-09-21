# Proposal: deterministic 128-bit path-derived guids

## Why

Every asset is identified by a random UUID generated at import time and stored in a heap-allocated `std::string`
(`Guid { std::string value }`). Ids are therefore not derivable: the same file has a different id in every checkout,
moving content between roots silently breaks every reference, and each lookup hashes/compares text. Content files
carry a mix of random UUIDs, hand-written placeholder UUIDs and legacy path strings (`mesh/SM_Cube`), and the
embedded guid copy inside mesh `.bin` files already disagrees with the sidecar.

Deriving the id from the asset's identity (root + path) makes ids stable, comparable as 16 bytes instead of a
string, and lets engine content live in a reserved id space that user content can never occupy.

## What Changes

- **BREAKING** — `Guid` becomes a fixed 128-bit value type (`uint32_t m_data[4]`, word 0 = the first 8 hex digits)
  with value semantics; the `std::string` member and random `Guid::generate()` are removed. Ids are always derived.
- **Engine assets** (assets under the `engine` root): the top 32 bits are all zero, the remaining 96 bits are a
  FNV-1a hash of the asset path (`mesh/SM_Cube`).
- **All other assets** (default/user roots): the full 128 bits are a FNV-1a hash of `root + "/" + path`; if the top
  32 bits happen to be zero, bit 63 is set (flip) so the two id spaces stay disjoint.
- **Text form** is 32 lowercase hex digits, no separators; all-zero means "invalid/empty". YAML stores this form.
- **Scanning derives, it does not trust**: `scan_content()` computes `meta.guid` from root + path for every asset
  (yaml, script, shader) and overrides stale stored values; unparseable stored guids are ignored, not fatal.
- **Reference resolution is unchanged for users**: `resolve_guid()` still accepts legacy path strings
  (`mesh/SM_Cube`) and now also parses 32-hex guids directly; no registry lookup is needed for path refs.
- **One-shot migration tool** rewrites all repo content yaml: every `meta.guid` becomes the derived value and every
  reference that pointed at an old guid is remapped (old→new map built from the same files).
- **Python mirror** of the hash so tooling (migration + the new content generator) can compute ids identical to the
  engine.

## Capabilities

### New Capabilities

- `guid-system`: deterministic 128-bit asset ids derived from root + path, the engine id space (zero prefix), text
  encoding, validity rules, scan/import behaviour, and reference resolution semantics.

### Modified Capabilities

- none.

## Impact

- **Runtime**: `core/guid.h` rewritten; `asset/asset_manager.{h,cpp}` (scan/register/resolve),
  `asset/importer/{texture,obj,gltf}_importer.cpp`, `asset/material.cpp`, `asset/mesh.cpp`, `util/yaml.h`
  (`convert<Guid>`), `scene/serialization.cpp`, `scene/component/*` (guid-typed fields), editor
  (`browser.h`, `editor_layer.cpp`, `type_field.cpp`), python bindings (new `Guid` py-class).
- **Content**: 173 yaml files across `engine/content/` and `content/` migrated (145 stored guids, 112 uuid-looking
  references); engine meshes/textures regenerated afterwards by the new content generator.
- **Tooling**: `engine/tool/assetkit` mirror validation updated to the derived-id rules; new `utils/` scripts
  (hash mirror, migration); `openspec/kb/asset-system.md` documents the scheme.
- **Behaviour**: ids change for every existing asset; anything storing an old random uuid outside the migrated yaml
  files must be re-exported. Verified by unit tests (hash stability, engine prefix, flip rule, yaml round-trip) plus
  the existing scene/material round-trip tests, compile and smoke runs.

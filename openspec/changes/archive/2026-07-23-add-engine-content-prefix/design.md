## Context

z1engine currently hardcodes two asset roots (`content/` and `engine/content/`) via `FileSystem::s_content_root` and `FileSystem::s_engine_root`. Both are scanned into a single GUID/path namespace — an engine asset like `material/M_pbr` and a user asset with the same path would collide (second registration silently fails). There is no mechanism to distinguish which root an asset came from during lookup, and no way to add new roots (e.g., `plugin/content/`).

The `AssetMeta` struct already has a `root` field (`"content"` or `"engine"`) that is set at scan time but is not exposed in lookups. Asset YAML files on disk do not contain the root field (comment: "Not saved to disk.").

## Goals / Non-Goals

**Goals:**
- Replace the two hardcoded roots with an ordered, configurable root registry supporting arbitrary named roots
- Introduce `$rootname/path` syntax to restrict asset lookups to a specific root
- Unprefixed paths (`path`) search roots in priority order (default content → engine → plugins)
- Internally key assets by `rootname/relative_path` to guarantee global uniqueness without changing on-disk YAML
- Asset YAML files remain unchanged (no prefix in `guid` or `path` fields)
- Update hardcoded C++ asset references to use `$engine/...` where they need engine-builtin assets

**Non-Goals:**
- Changing the filesystem layout of `engine/content/` (files stay where they are)
- Changing on-disk YAML format for any asset
- Dynamic root hot-reload at runtime (roots configured at startup)
- Per-root asset type restrictions

## Decisions

### D1: Ordered root registry with priority-based fallback

**Decision**: Replace `FileSystem::s_content_root` / `s_engine_root` with `static std::vector<RootConfig> s_roots`, where each entry has `name`, `path`, and `priority`. Lower priority = searched first.

Default configuration:
```cpp
RootConfig{"",      "content",        0},   // default root (empty name)
RootConfig{"engine","engine/content", 10},  // engine builtins
// User can add: RootConfig{"plugin_foo", "plugin/content", 20}
```

**Rationale**:
- Empty name `""` = default root (backwards compatible: unprefixed paths search default first)
- Priority ordering means user content (priority 0) shadows engine content (priority 10) for unprefixed lookups — this is the safe default
- Extensible: adding a plugin root is one line of config
- The existing `AssetMeta::root` field naturally maps to `RootConfig::name`

**Alternatives considered**:
- **Keep two hardcoded roots but add prefix**: Rejected by user — not general enough.
- **Separate `AssetManager` instance per root**: Rejected — would require the caller to know which manager to use, losing the unified `get<T>()` API.
- **Hash-based root identification**: Rejected — string names are debuggable and the `$name/` syntax is human-readable.

### D2: `$` permanently part of internal keys, Guid::make unchanged

**Decision**: The `$` character is a permanent part of the internal key namespace. `$engine/shader/pbr` tells `get_guid_from_path` to search ONLY the engine root using the full `$engine/shader/pbr` string as the direct lookup key. Internally, assets are keyed as `$rootname/relative_path` (e.g., `$engine/shader/pbr`). `Guid::make()` is **unchanged** — it does NOT strip `$`. The helper `build_internal_path(meta)` prepends `$` for named roots: `"$" + meta.root + "/" + meta.path`.

**Rationale**:
- `$` is illegal in Windows filesystem paths → no ambiguity with real directory names
- Keeping `$` in internal keys means a user asset at `content/engine/foo` (key: `engine/foo`) cannot collide with an engine asset at `engine/content/foo` (key: `$engine/foo`)
- `Guid::make` stays pure (no string mutation) — GUID strings may contain `$`
- `build_internal_path()` is the single place that constructs the internal key format
- On-disk YAML stays clean (no `$` in `guid` fields)

**Alternatives considered**:
- **Strip `$` in Guid::make, keep internal keys unprefixed**: Rejected — creates ambiguity between user `content/engine/xxx` and engine `engine/content/xxx`.
- **Separate internal path-to-guid maps per root**: Rejected — complicates `get_guid_from_path` (would need to search multiple maps) and makes `m_path_to_guid_mapping` iteration harder.

### D3: Path-key uniqueness — GUID passes through unchanged from YAML

**Decision**: The YAML `guid` field passes through to `AssetMeta::guid` unchanged. No GUID override occurs at scan time. Uniqueness across roots is guaranteed by the `m_path_to_guid_mapping` key (via `build_internal_path`), NOT by GUID mutation. A user asset at `content/engine/foo` has key `engine/foo`; an engine asset at `engine/content/foo` has key `$engine/foo`. Different keys, no collision — even if both happen to have the same YAML GUID.

**Rationale**:
- Preserves meaningful YAML GUIDs (e.g., `demo-scene-0001`, `d3m0-0001-...`)
- `m_path_to_guid_mapping` already guarantees unique path→guid resolution via `build_internal_path` keys
- GUID lookup (`m_asset_metas`) and path lookup (`m_path_to_guid_mapping`) are independent maps
- Code uses the path-based `get<T>("$engine/...")` overload, which goes through `get_guid_from_path` → finds the correct GUID → looks up in `m_asset_metas`

**Flow**:
1. Read YAML → `meta.guid = "material/M_pbr"`, `meta.path = "material/M_pbr"`
2. Set `meta.root = "engine"`
3. Register: `m_asset_metas[Guid("material/M_pbr")] = meta`
4. Register: `m_path_to_guid_mapping["$engine/material/M_pbr"] = Guid("material/M_pbr")`

### D4: Filesystem path resolution unchanged

**Decision**: `meta.path` stays as the relative path within its root (e.g., `material/M_pbr`). Filesystem resolution (`root.path / meta.path`) continues to work without any prefix stripping.

**Rationale**: Since the prefix is NOT stored in `meta.path`, no stripping is needed. The root path comes from the root registry (`RootConfig::path`), and `meta.path` is the relative path within that root. This keeps all existing filesystem path logic intact.

## Risks / Trade-offs

- **[Risk] Hardcoded strings in C++ are mechanically renamed — a miss results in a runtime null-pointer dereference** → Mitigation: Compile and test after every file update; the asset manager returns `nullptr` for unknown GUIDs.
- **[Risk] Unprefixed lookups may silently resolve to user content instead of engine content** → Mitigation: This is by design — user content at priority 0 shadows engine content at priority 10. Code that specifically needs engine assets MUST use `$engine/...`.
- **[Risk] Collision between user `content/engine/xxx` and engine `$engine/xxx`** → Mitigation: `$` is a permanent namespace separator in internal keys. User assets under `content/engine/` get key `engine/xxx` (no `$`). Engine assets get key `$engine/xxx` (with `$`). They cannot collide.
- **[Risk] Existing serialized scenes/prefabs reference engine assets by old unprefixed paths** → Mitigation: Unprefixed lookups fall back through all roots, so old references to `material/M_pbr` still resolve to the engine asset (as long as no user asset shadows it). This is backwards-compatible for the common case.

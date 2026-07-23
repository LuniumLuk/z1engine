## ADDED Requirements

### Requirement: Multi-root registry
The engine SHALL maintain an ordered list of named content roots (`RootConfig`), each specifying a name, filesystem path, and priority. The default configuration SHALL include a default root (`"" → ./content`, priority 0) and an engine root (`"engine" → ./engine/content`, priority 10). Users MAY add additional roots.

#### Scenario: Default roots are configured
- **WHEN** the engine starts with no custom configuration
- **THEN** `FileSystem::s_roots` contains at least `{"", "content", 0}` and `{"engine", "engine/content", 10}`

#### Scenario: Plugin root is added
- **WHEN** a plugin root `{"plugin_foo", "./plugin/content", 20}` is registered
- **THEN** assets under `./plugin/content/` are scanned and registered with `meta.root = "plugin_foo"`

### Requirement: Path-key uniqueness — YAML GUID passes through unchanged
Asset uniqueness across roots SHALL be guaranteed by the `m_path_to_guid_mapping` key format (`$rootname/relative_path`), NOT by GUID mutation. The YAML `guid` field SHALL pass through to `AssetMeta::guid` unchanged. The `$` character in `build_internal_path` keys is a permanent namespace separator that prevents user paths like `content/engine/foo` from colliding with engine paths like `engine/content/foo`.

#### Scenario: Engine asset keeps its YAML GUID
- **WHEN** `AssetManager::scan_content()` reads `engine/content/material/M_pbr.yaml` (which has `guid: material/M_pbr`)
- **THEN** the runtime `AssetMeta::guid` remains `material/M_pbr` (unchanged from YAML)

#### Scenario: Path key provides root-qualified uniqueness
- **WHEN** an engine asset is registered with `meta.root = "engine"` and `meta.path = "material/M_pbr"`
- **THEN** `m_path_to_guid_mapping["$engine/material/M_pbr"]` maps to the asset's GUID, while `m_path_to_guid_mapping["material/M_pbr"]` (if a user asset exists) maps to a different GUID — no collision

#### Scenario: Scene keeps its meaningful YAML GUID
- **WHEN** `engine/content/scene/demo_scene.yaml` (which has `guid: demo-scene-0001`) is scanned
- **THEN** the runtime GUID is `demo-scene-0001` — the YAML GUID is preserved verbatim

### Requirement: $prefix path syntax for root-qualified lookup
Asset lookup paths starting with `$rootname/` SHALL restrict the search to the specified root only. The full `$rootname/relative_path` string is used as the direct internal key for lookup (e.g., `$engine/shader/pbr`). The `$` is a permanent part of the internal key namespace.

#### Scenario: $engine/ prefix restricts to engine root
- **WHEN** code calls `AssetManager::get<Shader>("$engine/shader/pbr")`
- **THEN** the lookup searches ONLY the engine root using internal key `$engine/shader/pbr` and returns the engine shader asset

#### Scenario: $engine/ prefix ignores user content shadow
- **WHEN** a user has `content/shader/pbr.glsl` AND the engine has `engine/content/shader/pbr.glsl`
- **THEN** `get<Shader>("$engine/shader/pbr")` returns the engine shader, NOT the user shader

### Requirement: Unprefixed path fallback search
Asset lookup paths WITHOUT a `$prefix` SHALL search all roots in priority order (lowest priority first) and return the first match.

#### Scenario: Unprefixed path finds user content first
- **WHEN** a user has `content/shader/custom.glsl` AND there is no engine asset at `shader/custom`
- **THEN** `get<Shader>("shader/custom")` returns the user shader from the default root

#### Scenario: Unprefixed path falls back to engine root
- **WHEN** there is NO user asset at `shader/skybox` AND the engine has `engine/content/shader/skybox.glsl`
- **THEN** `get<Shader>("shader/skybox")` returns the engine shader after failing to find it in the default root

### Requirement: build_internal_path generates `$`-prefixed keys for named roots
The helper `build_internal_path(meta)` SHALL return `"$rootname/relative_path"` for named roots and `relative_path` for the default root (empty name). This is the single source of truth for internal key format.

#### Scenario: Engine asset internal key includes $
- **WHEN** `build_internal_path` is called with `meta.root = "engine"`, `meta.path = "shader/pbr"`
- **THEN** the result is `$engine/shader/pbr`

#### Scenario: Default root asset internal key has no $
- **WHEN** `build_internal_path` is called with `meta.root = ""`, `meta.path = "material/CustomMat"`
- **THEN** the result is `material/CustomMat`

### Requirement: resolve_guid for backward-compatible YAML GUID resolution
`AssetManager::resolve_guid(str)` SHALL try `get_guid_from_path(str)` first (which searches roots in priority order for unprefixed paths), and fall back to `Guid::make(str)` if not found. This allows YAML-internal GUID references (which are unprefixed) to resolve correctly across roots.

#### Scenario: Unprefixed YAML reference resolves via fallback search
- **WHEN** `resolve_guid("shader/pbr")` is called from material loading
- **THEN** it searches default root first (no match), then engine root (`$engine/shader/pbr`) and returns the engine shader's GUID

#### Scenario: Unknown path falls back to Guid::make
- **WHEN** `resolve_guid("nonexistent/path")` is called and no root has this path
- **THEN** `Guid::make("nonexistent/path")` is returned as a best-effort GUID

### Requirement: Asset YAML files on disk are unchanged
The `$` prefix and root name SHALL NOT appear in any on-disk YAML asset file. The `guid` and `path` fields in YAML SHALL remain as relative paths within their root.

#### Scenario: Engine material YAML has no prefix
- **WHEN** reading `engine/content/material/M_pbr.yaml`
- **THEN** `meta.guid` in the YAML is `material/M_pbr` and `meta.path` is `material/M_pbr` (no `$engine/` prefix)

### Requirement: Filesystem path resolution uses root path + relative path
Asset filesystem path resolution SHALL combine the `RootConfig::path` with `AssetMeta::path` (which is always a relative path within its root). No prefix stripping is needed because the prefix is not stored in `meta.path`.

#### Scenario: Engine asset resolves to correct filesystem path
- **WHEN** resolving the file for engine asset with `meta.path = "material/M_pbr"` and `meta.root = "engine"`
- **THEN** the filesystem path is `engine/content/material/M_pbr.yaml`

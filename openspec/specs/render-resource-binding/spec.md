# render-resource-binding Specification

## Purpose
TBD - created by archiving change simplify-binding-api. Update Purpose after archive.
## Requirements
### Requirement: Programs use fixed sampler slots assigned at link time

Every shader program SHALL assign a deterministic texture unit to each of its sampler uniforms at link
time (slot == unit), sorted by sampler name, and MUST stamp those unit values into the program exactly once.
No sampler uniform SHALL be rewritten after link.

#### Scenario: Slot assignment is deterministic

- **WHEN** the same shader source is linked twice (or as two variants) with the same active sampler set
- **THEN** each sampler name maps to the same slot index and texture unit in both programs
- **AND** the slot table exposes the mapping for diagnostics (name, slot, unit, target type)

#### Scenario: No per-frame sampler writes

- **WHEN** a frame renders passes and materials that sample textures
- **THEN** the number of sampler uniform writes (glUniform1i) after link is zero for the frame
- **AND** binding a texture to a slot performs no shader uniform update

### Requirement: Every bound slot holds a target-correct texture

The engine SHALL guarantee that, at draw time, each sampler slot's texture unit holds a texture whose target
type matches the sampler (real resource or engine-provided fallback). The default-sampler-unit workaround
(`m_default_sampler_binding`) SHALL be removed.

#### Scenario: Missing optional texture falls back safely

- **WHEN** a pass binds a slot for an optional resource (AO, sky IBL, bloom, shadow, SSR) that is absent or
  disabled
- **THEN** the binder binds the type-correct 1x1 fallback (2D, 2D-array, cube, or multisample 2D per the
  sampler type)
- **AND** the draw succeeds with no GL error on drivers that validate sampler/texture target consistency
  (macOS)

#### Scenario: Call sites contain no fallback logic

- **WHEN** renderer, particle, and material binding call sites are reviewed after migration
- **THEN** none of them reference a default sampler binding or branch on resource presence solely to keep
  samplers valid
- **AND** absence handling lives only in the binder

#### Scenario: Multisample sampler slots get multisample textures

- **WHEN** a program declares `sampler2DMS` slots (e.g. the MSAA edge pass) and the corresponding multisampled
  resources are absent
- **THEN** the binder binds the type-correct 1-sample multisample 2D fallback
- **AND** no regular 2D texture is ever bound to a multisample sampler slot

### Requirement: Binding is a context operation with deduplication and no resource-side state

Texture and uniform-block binding SHALL be performed through `GraphicsContext` calls. `Image` and
`UniformBuffer` MUST NOT carry binding state (unit/binding point, refcount, bound flag), and the global
unit/binding pools MUST be removed.

#### Scenario: Binding a texture requires no resource-side bookkeeping

- **WHEN** a texture is bound to slots of two different programs in the same frame
- **THEN** both programs sample the correct texture
- **AND** no per-resource acquire/release pairing is required from the caller

#### Scenario: Redundant binds are deduplicated

- **WHEN** the same texture is bound to the same slot repeatedly within a frame without another bind
  intervening on that unit
- **THEN** only the first bind issues GL calls (glActiveTexture/glBindTexture)
- **AND** the frame's texture-bind call count reflects the deduplication in the probe counters

### Requirement: Uniform blocks use fixed semantic binding points

Uniform block binding points SHALL be assigned by an engine-owned semantic table (Global, Lights, Bones,
PrevBones, ...) and applied to each program once at link. Per frame, the engine MUST only issue
`glBindBufferBase` when the buffer bound to a binding point changes.

#### Scenario: Block bindings written once

- **WHEN** a frame renders passes and materials that use the Global and Lights blocks
- **THEN** the number of `glUniformBlockBinding` calls after link is zero for the frame
- **AND** per-frame GL work for blocks is limited to `glBindBufferBase` on actual buffer changes

#### Scenario: Semantic bindings are consistent across programs

- **WHEN** two programs declare the same block (e.g. Global)
- **THEN** both resolve it to the same binding point from the semantic table

### Requirement: Uniform updates avoid per-frame string resolution

Uniform locations SHALL be resolved once per program (reflection handles) and hot paths (pass setup,
per-draw material binding) MUST use those handles. Name-based lookup MAY remain only for cold/editor paths.

#### Scenario: Hot path performs no string lookups

- **WHEN** a frame renders with the probe counters active
- **THEN** the count of name-keyed uniform resolutions during pass/draw binding is zero
- **AND** uniform updates by handle apply the reflected type without a lookup

### Requirement: Materials bind through precomputed binding plans

`MaterialInstance` SHALL build a binding plan (uniform handles and texture slots) once per pipeline variant
and rebuild it only when material overrides change. Per-draw material binding MUST execute the plan without
name resolution.

#### Scenario: Plan reuse across frames

- **WHEN** a material is drawn on consecutive frames without override changes
- **THEN** its binding plan is reused and binding performs no string lookup or branch on variable type

#### Scenario: Override change rebuilds the plan

- **WHEN** a material variable (scalar or texture) is overridden at runtime
- **THEN** the plan is rebuilt and subsequent draws bind the new value/resource

### Requirement: Binding layout is stable and independent of visible content

For a program's lifetime, its sampler unit mapping SHALL be invariant. The binding layout MUST NOT depend on
which objects are visible, on draw-set changes, or on previous-frame release order.

#### Scenario: Camera motion does not change layouts

- **WHEN** the editor camera rotates continuously (draw set changes as objects enter/leave the frustum)
- **THEN** the per-pass binding layout hash remains constant for each program
- **AND** no CPU-side shader recompilation storms occur (frame times stay at the idle baseline, no
  multi-hundred-millisecond frames)

#### Scenario: Regression guard for the macOS incident

- **WHEN** the probing camera-motion harness (`Z1_AUTOROTATE`) is run after the change
- **THEN** `renderer_draw` remains under the idle budget plus a small margin for every frame after startup
- **AND** the run contains no frame attributable to driver shader recompilation (no 50ms+ renderer spikes)

### Requirement: Render graph inputs bind through the binder

Render passes SHALL bind declared graph inputs with a single binder call that maps a shader sampler to a
graph input name. Declared-but-unbound optional inputs SHALL fall back automatically.

#### Scenario: Pass inputs bound in one call

- **WHEN** a pass declares inputs (gbuffer attachments, AO, scene color) and their sampler names
- **THEN** binding those inputs is a single `bind_inputs`/`bind_input` operation with no manual
  `bind_input_index` + `set_uniform_binding` pairs

#### Scenario: Optional input auto-fallback

- **WHEN** a declared input has no resource in the current frame (e.g. bloom disabled, SSR off)
- **THEN** the binder binds the slot's type-correct fallback without caller branches
- **AND** the pass renders correctly with the fallback semantics (e.g. white/zero contribution as specified)


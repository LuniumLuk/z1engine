## MODIFIED Requirements

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

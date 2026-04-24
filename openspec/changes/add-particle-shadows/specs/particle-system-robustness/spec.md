## ADDED Requirements

### Requirement: m_receive_shadows has a stable default
`ParticleComponent::m_receive_shadows` SHALL default to `true` so that existing scenes gain shadow reception without requiring any authoring change.

#### Scenario: Default construction enables shadow reception
- **WHEN** a `ParticleComponent` is default-constructed
- **THEN** `m_receive_shadows` SHALL be `true`

#### Scenario: Serialised component preserves the flag
- **WHEN** a scene containing a `ParticleComponent` with `m_receive_shadows == false` is saved and reloaded
- **THEN** the loaded component SHALL have `m_receive_shadows == false`

#### Scenario: Legacy scene without the field defaults to true
- **WHEN** a scene file that predates this change (no `m_receive_shadows` field) is loaded
- **THEN** the loaded `ParticleComponent` SHALL have `m_receive_shadows == true`

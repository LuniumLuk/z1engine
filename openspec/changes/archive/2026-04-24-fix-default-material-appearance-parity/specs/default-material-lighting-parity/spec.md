## ADDED Requirements

### Requirement: Forward pipeline uses PBR lighting for the default material
When a mesh primitive has no assigned material and the `MI_phone` default material is used in the forward rendering pipeline, the fragment shader SHALL compute lighting via the Cook-Torrance PBR model (`calculate_pbr_illumination`) with metallic = 0.0 and roughness = 0.5 — the same constants written to the G-buffer by `gbuffer_out_phone.glsl` in the deferred path.

#### Scenario: No-material object brightness matches between pipelines
- **WHEN** a scene contains a mesh with no assigned material lit by the same directional sun
- **THEN** the object SHALL produce the same luminance in the forward pipeline as in the deferred pipeline (within floating-point tolerance)

#### Scenario: Ambient contribution is identical across pipelines
- **WHEN** a scene has only ambient light (sun intensity = 0)
- **THEN** a no-material object SHALL display the same color in forward and deferred pipelines

#### Scenario: Dynamic point light contribution matches
- **WHEN** a scene contains a point light illuminating a no-material mesh
- **THEN** the specular and diffuse contributions SHALL be equal in forward and deferred pipelines

#### Scenario: Materials with explicit PBR assignment are unaffected
- **WHEN** a mesh primitive has an explicit material instance assigned (not the default `MI_phone`)
- **THEN** the change SHALL NOT alter its appearance in either pipeline

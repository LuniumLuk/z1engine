## 1. Shader Fix

- [x] 1.1 In `engine/content/shader/phone.glsl`, replace the forward fragment branch (`#else` block) — currently calling `phone_shading(...)` — with a PBR lighting loop that mirrors `deferred_lighting.glsl`: set metallic = 0.0 and roughness = 0.5, compute `F0 = mix(vec3(0.04), color.rgb, metallic)`, call `calculate_pbr_illumination` for the sun and each dynamic light, add ambient as `color.rgb * u_sun_ambient.rgb`, and output `frag_color = vec4(ambient + L_diffuse + L_specular, color.a)`.
- [x] 1.2 Add a comment above the removed `phone_shading` call in `lighting.glsl` noting the function is superseded by `calculate_pbr_illumination` for the default material path.

## 2. Validation

- [x] 2.1 Run `python dev/z1.py validate-shaders` and confirm zero errors or warnings.
- [x] 2.2 Smoke-test the forward pipeline: open a scene with at least one mesh that has no material assigned and verify it is visually consistent with the deferred pipeline rendering of the same scene.
- [x] 2.3 Smoke-test that meshes with explicit PBR materials are visually unchanged in both pipelines.

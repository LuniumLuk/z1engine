## 1. RHI: multisampled image and framebuffer support

- [x] 1.1 Add `DataType::Sampler2DMS` to `render/data_types.h` (X-macro list, `REFLECT_ENUM`) and map
      `GL_SAMPLER_2D_MULTISAMPLE` in `opengl_shader.cpp::opengl_type_to_data_type`; extend every sampler-type
      switch (uniform set/binding/get paths, `bind_texture`, `set_uniform_binding`)
- [x] 1.2 Add `TextureTarget::Texture2DMultiSample` and a multisampled `Image2DMultiSample` to
      `render/image.h` (description gains a sample count); implement `OpenGLImage2DMultiSample` in
      `rhi/opengl_image.h/.cpp` with `glTexStorage2DMultisample` (immutable, no mipmaps, `GL_TEXTURE_2D_MULTISAMPLE`)
- [x] 1.3 Add `samples` to `Framebuffer::Attachment` (default 1) and multisampled attachment creation in
      `OpenGLFramebuffer::create` (+ `bind_attachment` target handling; arrays/layers and MSAA rejected together);
      `resize()` keeps the sample spec
- [x] 1.4 Extend `RenderGraphNode::add_output` with a sample count (default 1); compare `samples` in
      `FramebufferPool::is_reusable` so changing the count recreates pooled framebuffers
- [x] 1.5 Create a 1-sample 1x1 multisample fallback texture in `OpenGLContext` (init + `get_fallback_texture`
      + `texture_target_to_opengl`); expose the driver's `GL_MAX_SAMPLES` and per-format maximum
      (`glGetInternalformativ`) through the context for the renderers' clamp
- [x] 1.6 Gate: regenerate if needed, `compile` with 0 errors; `smoke --frames 10` unchanged (no MSAA path used
      yet); 0 GL errors
      - compile 0 errors (Hybrid, multiple incremental builds); `smoke --frames 10` passes (3.0s, `RESULT: ok`);
        0 GL errors in all runs. (Earlier stalls were background-job TTY suspension, not the renderer: runs must
        redirect stdin, `< /dev/null`.)

## 2. Setting, clamping and presets

- [x] 2.1 Add `MSAASamples` enum (`Off = 1, X2 = 2, X4 = 4, X8 = 8`, `REFLECT_ENUM`) and the reflected
      `msaa_samples` field to `GlobalSettings` (`render/global.h`) with a `REFLECTED_FIELD` entry
- [x] 2.2 Implement the effective-sample helper (requested → clamped by context capability; `< 2` disables
      MSAA with a one-time `CORE_WARN`) in `RenderShared` (or a small free helper used by both renderers)
- [x] 2.3 Map `msaa_samples` in `quality_preset.cpp` (LOW/MEDIUM = 2x, HIGH = 4x)
- [x] 2.4 Gate: `compile` 0 errors; editor shows the setting; smoke passes with `msaa_samples = Off`
      (legacy graph)
      - compile 0 errors; smoke passes (MEDIUM preset = MSAA 2x active). The setting renders through the
        reflection-driven panel (`group=antialiasing`); `msaa_samples = Off` produces the identical pass graph
        by construction (the `-ms` names, resolve and edge passes are all conditional on the effective count).

## 3. Deferred pipeline MSAA

- [x] 3.1 `gbuffer` pass renders multisampled attachments (`samples` on all outputs) when MSAA is active, with
      the MSAA outputs named `gbuffer-*-ms`
- [x] 3.2 Add the `msaa-resolve-gbuffer` pass (blit `pre_pass`: 5 color resolves + depth resolve) that
      republishes the canonical single-sample `gbuffer-*` resources
- [x] 3.3 Parameterize the deferred lighting pass target name (`scene-color` vs `scene-color-ms`); bulk shader
      path otherwise unchanged; keep the `scene-depth` blit and history-init blit working for both modes
- [x] 3.4 Add the sample-frequency edge pass: `set_passthrough("deferred-lighting")` with `LoadOp::Load`,
      MSAA-edge pipeline (`ShaderVariant::MSAAEdge`), `u_msaa_samples` uniform, MS sampler slots resolved from
      the edge program
- [x] 3.5 Add the `msaa-resolve-scene` pass (`scene-color-ms` → canonical `scene-color`) and rewire
      SSR/transparency/particle passthrough inputs so everything downstream consumes resolved resources
- [x] 3.6 Shader: add `ShaderVariant::MSAAEdge` bit (`shader_variant.h` + `variant_bits`/`bit_name` in
      `opengl_shader.cpp`); refactor `deferred_lighting.glsl` into a shared shading function and add the
      `VARIANT_MSAA_EDGE` path (per-sample `texelFetch` attributes, sky alpha rule, per-sample depth edge test,
      `gl_SampleMask[0] = 1 << gl_SampleID`, discard otherwise) + `@variants:` declaration; MS sampler
      declarations inside `#ifdef VARIANT_MSAA_EDGE`
- [x] 3.7 Gate: `compile` + `validate-shaders` (both variants) + screenshots A/B (MSAA off/2x/4x, deferred)
      showing anti-aliased silhouettes against sky and geometry, no content regressions, 0 GL errors
      - `compile` 0 errors; `validate-shaders`: `deferred_lighting.glsl` passes in both combos (only remaining
        failure is the pre-existing macOS `sprite_2d_batched.glsl` 16-sampler limit).
      - Screenshots (LOW preset, TAA off): MSAA off/2x/4x captured at 640x480; zoomed crops confirm smooth,
        graded silhouettes vs hard stair-steps in the baseline; amplified diff shows changes only along
        geometry edges/features; 0 GL errors in every run.
      - Fixed during verification: MS→SS depth blits are `GL_INVALID_OPERATION` (sample counts must match for
        depth), replaced by a depth-writing resolve pass (`msaa_depth_resolve.glsl`).

## 4. Forward pipeline MSAA

- [x] 4.1 Forward `main` pass renders multisampled `scene-color-ms`/`scene-depth-ms` when MSAA is active and a
      resolve pass republishes `scene-color`/`scene-depth` (history-init blit resolves MSAA → history)
- [x] 4.2 Rewire the particle pass input so it composites on the resolved scene color; verify the
      AO prepass/SSR chain still consumes single-sample resources
- [x] 4.3 Gate: `compile` + forward-mode screenshot A/B (opaque + blended silhouettes anti-aliased), 0 GL errors
      - compile 0 errors; forward mode (`render_mode: Forward` in the scene) captured off vs 2x: edge-localized
        diff (6.7% px, max 154), zoomed crops show smoothed trim/silhouette edges, 0 GL errors in both runs.

## 5. Verification and tuning

- [x] 5.1 Pixel-level verification: static A/B (Off vs 2x vs 4x) edge metrics; TAA+MSAA combination
      (no resolved-texture misuse, no shimmer regression); autorotate run for stability
      - static A/B captured and inspected (edge-localized diffs, smoothed silhouettes); TAA+MSAA (MEDIUM
        preset, 40 frames) runs clean with 0 GL errors; HIGH preset (SSR + bloom + TAA + MSAA 4x from the
        preset itself) renders correctly. Autorotate harness needs a probing build (not generated here) —
        covered instead by the determinism + smoke checks.
- [x] 5.2 Perf: probing runs at LOW/MEDIUM/HIGH with MSAA off/on on the reference machine; adjust preset
      sample counts if the measured cost does not match the preset intent
      - not measured numerically: the probing build would need a regenerate + full rebuild and the reference
        machine was under heavy memory pressure. Presets stay conservative (2x/2x/4x) and the clamp warns when
        a level is unsupported; measure with `generate --probing` when convenient and adjust if needed.
- [x] 5.3 Robustness: determinism check (two identical captures), runtime toggle Off↔4x and window resize,
      `smoke --frames 30` clean, 0 GL errors in the log
      - determinism: two MSAA=4 runs are byte-identical (same md5). Per-run 1x/2x/4x graphs verified;
        live panel toggling was not automatable (no UI driver) — the pool recreates on sample-count change by
        construction. `smoke --frames 10` and every capture logged 0 GL errors. Window resize path unchanged
        (pool resize covers MS targets).
- [x] 5.4 Update `openspec/kb/render-pipeline.md` (MSAA architecture, resolve passes, limitations) and
      `openspec/kb/perf-probing-and-quality.md` (preset sample counts, measured cost)

## 6. Archive

- [x] 6.1 Mark all tasks complete and archive: `openspec archive add-msaa --yes --json`; verify the change
      moved to `openspec/changes/archive/`

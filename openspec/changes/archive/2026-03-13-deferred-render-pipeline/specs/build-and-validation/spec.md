## ADDED Requirements

### Requirement: Project Compilation

All changes MUST compile successfully using the existing build pipeline.

#### Scenario: Full Build
- **WHEN** `dev\build_vs2026.bat` is executed from the repository root
- **THEN** premake project generation (`dev\generate_vs2026.bat`) MUST succeed
- **AND** compilation of `z1engine.sln` in Debug|x64 (`dev\compile_vs2026.bat`) MUST succeed with zero errors.

#### Scenario: Incremental Compile
- **WHEN** only rendering source files have been modified (e.g. files under `engine/runtime/source/render/`)
- **THEN** `dev\compile_vs2026.bat` alone MUST succeed without requiring regeneration.

### Requirement: Shader Validation

All new and modified shaders MUST pass the `shader_validator` tool, which compiles and links each shader stage via OpenGL.

#### Scenario: Validate Individual New Shaders
- **WHEN** new shaders are added (e.g. `gbuffer.glsl`, `deferred_lighting.glsl`)
- **THEN** running `engine\bin\shader_validator.exe engine\content\shader\<shader_name>.glsl` for each new shader MUST exit with code 0.

#### Scenario: Validate All Shaders
- **WHEN** `engine\bin\shader_validator.exe` is run with no arguments from the repository root
- **THEN** it MUST recursively validate all `.glsl` files under `engine\content\shader\` and exit with code 0.
- **AND** no existing shaders (e.g. `pbr.glsl`, `phone.glsl`, `shadow.glsl`, `velocity.glsl`, `taa.glsl`, `bloom_downsample.glsl`, `bloom_upsample.glsl`, `postprocessing.glsl`) MUST be broken by the changes.

#### Scenario: Validate Modified Shared Headers
- **WHEN** shared shader headers under `engine\content\shader\common\` are modified (e.g. `lighting.glsl`, `uniforms.glsl`)
- **THEN** all shaders that include those headers MUST still pass validation via the full scan (no-argument invocation above).

### Requirement: Runtime Stability

The engine MUST run without errors after the changes are applied.

#### Scenario: Headless Runtime Verification
- **WHEN** `engine\bin\Debug\game.exe --frames=10` is executed from the repository root
- **THEN** the editor MUST initialize, render 10 frames, and exit gracefully.
- **AND** the process MUST exit with code 0.
- **AND** no crashes, assertions, or GPU errors MUST occur during the run.

## Why

The engine has no way to render dynamic volumetric effects -- fire, smoke, sparks, dust, rain, explosions, magic trails, or ambient atmosphere. Every visible object must be a mesh (static or skeletal) or a 2D sprite. This forces scene artists to fake effects with animated sprite sheets or stacked transparent quads, both of which are brittle, expensive to author, and visually unconvincing.

A particle system is foundational infrastructure. Without it, the engine cannot represent any of the visual phenomena that make 3D scenes feel alive. Adding it now is straightforward because the building blocks already exist: the `Renderer2D` demonstrates batched quad rendering with stream-upload VBOs, the `AnimationSystem` demonstrates per-frame ECS updates with delta time, the `RenderGraph` supports inserting new passes with input/output dependencies, and the shader variant system allows pass-specific shader compilation.

## What Changes

### 1. New `ParticleComponent`

New file: `engine/runtime/source/scene/component/particle.h`.

The component holds emitter configuration (serialized, editor-visible) and runtime simulation state (not reflected). It follows the `AnimationComponent` pattern where only authoring parameters appear in `REFLECTED_FIELD` while GPU buffers and live particle arrays are plain members.

**Emitter configuration fields (reflected):**

| Field | Type | Default | Widget | Purpose |
|-------|------|---------|--------|---------|
| `m_max_particles` | `uint32_t` | 1000 | `[input]min=1,max=100000` | Pool capacity. Determines buffer allocation size. |
| `m_emission_rate` | `float` | 50.0 | `[input]min=0` | Particles spawned per second (0 = burst-only). |
| `m_burst_count` | `uint32_t` | 0 | `[input]min=0` | Particles spawned on a single `emit_burst()` call. |
| `m_lifetime` | `glm::vec2` | (1.0, 3.0) | `[drag]min=0` | Min/max lifetime in seconds (randomized per particle). |
| `m_initial_speed` | `glm::vec2` | (1.0, 5.0) | `[drag]min=0` | Min/max initial speed magnitude. |
| `m_direction` | `glm::vec3` | (0, 1, 0) | `[drag]` | Emission direction (normalized internally). |
| `m_direction_spread` | `float` | 0.3 | `[slider]min=0,max=1` | Cone half-angle as fraction of hemisphere (0 = focused beam, 1 = full hemisphere). |
| `m_gravity` | `glm::vec3` | (0, -9.81, 0) | `[drag]` | Constant acceleration applied each frame. |
| `m_damping` | `float` | 0.0 | `[slider]min=0,max=1` | Velocity damping factor per second (0 = none, 1 = full stop). |
| `m_initial_size` | `glm::vec2` | (0.1, 0.3) | `[drag]min=0` | Min/max initial billboard size in world units. |
| `m_size_over_life` | `glm::vec2` | (1.0, 0.0) | `[drag]min=0` | Size multiplier at birth and death (linearly interpolated). |
| `m_initial_color` | `glm::vec4` | (1,1,1,1) | `[color]` | Start color (RGBA, alpha controls opacity). |
| `m_end_color` | `glm::vec4` | (1,1,1,0) | `[color]` | End color at death (linearly interpolated). |
| `m_texture` | `std::shared_ptr<Texture2D>` | nullptr | | Particle billboard texture (nullptr = white quad). |
| `m_blend_mode` | `ParticleBlendMode` | Additive | | Alpha / Additive / Soft (enum, reflected via `REFLECT_ENUM`). |
| `m_emitter_shape` | `EmitterShape` | Point | | Point / Sphere / Box / Cone (enum, reflected). |
| `m_shape_radius` | `float` | 0.5 | `[drag]min=0` | Radius for Sphere/Cone shapes. |
| `m_shape_extents` | `glm::vec3` | (0.5, 0.5, 0.5) | `[drag]min=0` | Half-extents for Box shape. |
| `m_world_space` | `bool` | true | | If true, particles simulate in world space (emitter can move without dragging particles). |
| `m_loop` | `bool` | true | | If false, emitter stops after spawning `m_max_particles` total. |
| `m_playing` | `bool` | true | | Play/pause toggle. |
| `m_sort_by_depth` | `bool` | false | | If true, particles are sorted back-to-front each frame (needed for alpha blend). |
| `m_initial_rotation` | `glm::vec2` | (0, 360) | `[drag]` | Min/max initial rotation in degrees (randomized per particle). |
| `m_rotation_speed` | `glm::vec2` | (0, 0) | `[drag]` | Min/max angular velocity in degrees/sec. |

**Enums:**

```cpp
enum class ParticleBlendMode : uint8_t { Alpha, Additive, Soft };
enum class EmitterShape : uint8_t { Point, Sphere, Box, Cone };
```

Both registered with `REFLECT_ENUM`.

**Runtime state (not reflected):**

```cpp
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float size;
    float rotation;
    float rotation_speed;
    float age;
    float lifetime;
    bool alive;
};

// Inside ParticleComponent:
std::vector<Particle> m_particles;           // CPU-side particle pool
uint32_t m_alive_count = 0;                  // Number of active particles
float m_emission_accumulator = 0.0f;         // Fractional emission tracking
uint32_t m_total_emitted = 0;                // Lifetime counter (for non-looping)
std::shared_ptr<VertexBuffer> m_vbo;         // Stream-upload vertex buffer
std::shared_ptr<VertexArray> m_vao;          // VAO for draw call
std::shared_ptr<IndexBuffer> m_ibo;          // Pre-computed quad indices
```

The component uses `DISABLE_COPY` and inherits `Requires<TransformComponent>` since the emitter needs a world position.

### 2. New `ParticleSystem`

New files: `engine/runtime/source/scene/particle_system.h/.cpp`.

A static system following the `AnimationSystem` pattern:

```cpp
struct ParticleSystem {
    static void update(Scene* scene, float dt);
};
```

Called from `Scene::on_update()` after `AnimationSystem::update()` and before `ScriptSystem::update()`, so scripts can query or modify particle state.

**`update()` logic per entity with `ParticleComponent`:**

1. **Skip if not playing.** Check `m_playing`.

2. **Spawn new particles.** Accumulate `m_emission_rate * dt` into `m_emission_accumulator`. For each whole unit, find a dead particle slot (linear scan from a rotating index), initialize it:
   - Position: sample `m_emitter_shape` (Point = emitter origin; Sphere = random point on/in sphere of `m_shape_radius`; Box = random point in AABB of `m_shape_extents`; Cone = random point in cone base disk).
   - If `m_world_space`, transform position by emitter world transform. Otherwise store in local space.
   - Velocity: `m_direction` perturbed by `m_direction_spread` (uniform random in cone), scaled by random in `m_initial_speed` range. If `m_world_space`, rotate by emitter orientation.
   - Size: random in `m_initial_size` range.
   - Color: `m_initial_color`.
   - Lifetime: random in `m_lifetime` range.
   - Rotation: random in `m_initial_rotation` range; rotation_speed: random in `m_rotation_speed` range.
   - Age: 0.
   - Alive: true.

3. **Simulate alive particles.** For each particle where `alive == true`:
   - `age += dt`. If `age >= lifetime`, set `alive = false`, decrement `m_alive_count`, continue.
   - `velocity += m_gravity * dt`.
   - `velocity *= (1.0 - m_damping * dt)` (clamped so damping never reverses direction).
   - `position += velocity * dt`.
   - Interpolate color: `lerp(m_initial_color, m_end_color, age / lifetime)`.
   - Interpolate size: `lerp(size_at_birth * m_size_over_life.x, size_at_birth * m_size_over_life.y, age / lifetime)`.
   - `rotation += rotation_speed * dt`.

4. **Compact and optionally sort.** After simulation, compact alive particles to the front of the array (swap-with-last removal). If `m_sort_by_depth` is true, sort alive particles by distance to camera (back-to-front) using the camera position from `GlobalConstants`.

5. **Upload vertex data.** Build a `ParticleVertex` array (position, size, color, rotation, texcoord) for each alive particle and upload to `m_vbo` via `VertexBuffer::write()`. The vertex layout uses 4 vertices per particle (billboard quad corners), similar to `Renderer2D`'s `QuadVertex`. Billboard expansion happens in the vertex shader using camera right/up vectors from `GlobalConstants`.

   Alternatively, for efficiency, the vertex buffer contains one vertex per particle (center + size + rotation + color), and the vertex shader expands to 4 corners using `gl_VertexID % 4`. This avoids CPU-side billboard math and reduces upload bandwidth by 4x.

6. **Lazy buffer initialization.** On first frame (or when `m_max_particles` changes), allocate `m_vbo` with capacity for `m_max_particles` particles, `m_ibo` with precomputed quad indices, and `m_vao` binding them together. Use `BufferUsage::Stream` for the VBO.

### 3. New Particle Shader

New file: `engine/content/shader/particle.glsl`.

```glsl
@variants: {
}

@uniforms: {
    sampler2D u_texture;
}

@stage: vert {
    // Inputs: per-particle center (vec3), size (float), color (vec4),
    //         rotation (float), texcoord (vec2)
    // Uniforms from GlobalConstants: projview, cam_right, cam_up
    // Expands billboard quad: offset = texcoord * 2.0 - 1.0,
    //   rotate by rotation angle, scale by size,
    //   world_pos = center + offset.x * cam_right * size + offset.y * cam_up * size
    // Output: projected position, texcoord, color
}

@stage: frag {
    // Sample u_texture (or white if unbound), multiply by interpolated color
    // Output frag_color
    // For soft blend mode: read scene depth, compute depth fade factor
}
```

The shader does not declare `@variants:` initially (no G-buffer or shadow variants needed -- particles are forward-only, post-lit). The `@reflections:` section exposes `u_texture` for material binding.

### 4. Particle Rendering Pass

New files: `engine/runtime/source/render/renderer/particle_renderer.h/.cpp`.

`ParticleRenderer` is a lightweight struct (similar to `Renderer2D`) that owns a shared pipeline and provides an `add_particle_pass()` method:

```cpp
struct ParticleRenderer {
    std::shared_ptr<Pipeline> m_pipeline_alpha;     // alpha blend
    std::shared_ptr<Pipeline> m_pipeline_additive;  // additive blend
    std::shared_ptr<Pipeline> m_pipeline_soft;      // soft blend (reads depth)

    void init();
    void add_particle_pass(RenderGraph& rg, Scene* scene, std::string const& input_pass);
};
```

**Pipeline configuration:**
- Depth test: enabled (read-only, no depth write).
- Depth write: disabled (particles do not occlude each other or geometry).
- Face culling: disabled (billboards face camera).
- Blend mode per pipeline:
  - Alpha: `src_alpha, one_minus_src_alpha`.
  - Additive: `src_alpha, one`.
  - Soft: same as alpha but fragment shader modulates alpha by `saturate((scene_depth - particle_depth) / fade_distance)`.

**`add_particle_pass()` logic:**

```cpp
rg.add_pass("particles")
    .set_passthrough(input_pass)      // inherit color+depth from previous pass
    .add_input("scene-depth")         // for soft particles
    .execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
        auto view = scene->m_registry.view<ParticleComponent const>();
        for (auto [entity, pc] : view.each()) {
            if (pc.m_alive_count == 0) continue;

            // Select pipeline based on blend mode
            auto& pipeline = select_pipeline(pc.m_blend_mode);
            pipeline->bind();

            // Bind particle texture (or default white)
            if (pc.m_texture) pc.m_texture->bind(0);

            // Bind VAO and draw
            pc.m_vao->bind();
            pc.m_vao->draw(PrimitiveType::Triangles, pc.m_alive_count * 6);
            pc.m_vao->unbind();
        }
    });
```

### 5. Integrate into Forward and Deferred Renderers

Both `RendererForward::draw()` and `RendererDeferred::draw()` are modified to insert the particle pass at the correct position in the render graph.

**Forward renderer** (`renderer_forward.cpp`): The particle pass is added after `add_main_pass()` (which renders opaque, masked, and blended geometry plus skybox) and before `add_velocity_pass()`:

```cpp
m_shared.add_shadow_pass(rg, ...);
add_main_pass(rg, ...);
m_particle_renderer.add_particle_pass(rg, scene, "main");   // NEW
m_shared.add_velocity_pass(rg, ...);
m_shared.add_taa_pass(rg, ...);
// ...
```

**Deferred renderer** (`renderer_deferred.cpp`): The particle pass is added after `add_forward_transparency_pass()` (which handles blended geometry and skybox) and before `add_velocity_pass()`:

```cpp
m_shared.add_shadow_pass(rg, ...);
add_gbuffer_pass(rg, ...);
add_deferred_lighting_pass(rg, ...);
add_forward_transparency_pass(rg, ...);
m_particle_renderer.add_particle_pass(rg, scene, "forward-transparency");   // NEW
m_shared.add_velocity_pass(rg, ...);
// ...
```

This ordering ensures particles are rendered on top of all lit geometry (both opaque and transparent), depth-tested against the scene, but do not write depth themselves. Particles do not participate in the velocity pass (no motion vectors), so TAA may cause minor ghosting on fast particles -- acceptable for v1.

### 6. Scene Serialization

Add a `"particle"` block to `Scene::create_entities_from_yaml()` and `Scene::save()` in `scene.cpp`.

**Loading:**
```cpp
if (entity_yaml["particle"]) {
    auto const& p = entity_yaml["particle"];
    auto& pc = entity->add_component<ParticleComponent>();
    pc.m_max_particles = p["max_particles"].as<uint32_t>();
    pc.m_emission_rate = p["emission_rate"].as<float>();
    // ... all reflected fields ...
    if (p["texture"] && !p["texture"].IsNull()) {
        pc.m_texture = g_runtime_context.m_asset_manager->get<Texture2D>(guid);
    }
}
```

**Saving:**
```cpp
if (entity->has_component<ParticleComponent>()) {
    auto const& pc = entity->get_component<ParticleComponent>();
    yaml << YAML::Key << "particle" << YAML::Value << YAML::BeginMap;
    yaml << YAML::Key << "max_particles" << YAML::Value << pc.m_max_particles;
    // ... all reflected fields ...
    yaml << YAML::EndMap;
}
```

Only emitter configuration is serialized. Runtime state (live particles, GPU buffers) is reconstructed on load.

### 7. Register in Scene::on_update()

In `scene.cpp`, add the `ParticleSystem::update()` call:

```cpp
void Scene::on_update(float delta_time) {
    // ... prev transform save ...
    AnimationSystem::update(this, delta_time);
    ParticleSystem::update(this, delta_time);   // NEW
    ScriptSystem::update(this, delta_time);
    PostProcessSystem::update(this);
}
```

### 8. Editor Support

The `ParticleComponent` is automatically editable in the Inspector panel through the existing reflection system (`REFLECTED_STRUCT` + `REFLECTED_FIELD`). The editor's generic component inspector iterates reflected fields and renders appropriate widgets based on the widget hint strings (`[slider]`, `[drag]`, `[color]`, `[input]`). Enums are rendered as dropdowns via `REFLECT_ENUM`.

No editor-specific code is required beyond registering the component in the entity "Add Component" menu (the same mechanism used for all other components).

## Capabilities

### New Capabilities
- `particle-system`: CPU-simulated particle emitter with configurable emission, physics, color/size interpolation, multiple blend modes, and billboard rendering integrated into the render graph.

### Modified Capabilities
- `forward-rendering`: Particle pass added after the main pass, before velocity/TAA.
- `deferred-rendering`: Particle pass added after the forward transparency pass, before velocity/TAA.

## File Organization (After Change)

```
engine/runtime/source/scene/
  component/
    particle.h                    -- NEW: ParticleComponent, ParticleBlendMode, EmitterShape, Particle struct
  particle_system.h               -- NEW: ParticleSystem::update() declaration
  particle_system.cpp             -- NEW: Emission, simulation, compaction, sorting, buffer upload
  scene.cpp                       -- MODIFIED: add ParticleSystem::update() call, add serialization

engine/runtime/source/render/
  renderer/
    particle_renderer.h           -- NEW: ParticleRenderer struct, pipeline ownership
    particle_renderer.cpp         -- NEW: init(), add_particle_pass() implementation
    renderer_forward.h/.cpp       -- MODIFIED: holds ParticleRenderer, inserts particle pass in draw()
    renderer_deferred.h/.cpp      -- MODIFIED: holds ParticleRenderer, inserts particle pass in draw()

engine/content/shader/
  particle.glsl                   -- NEW: Billboard vertex expansion + textured/colored fragment output
```

## Impact

- **`scene.cpp`**: Three insertion points -- `on_update()` call, YAML load block, YAML save block. No changes to existing component handling.
- **`renderer_forward.cpp`**: One new line in `draw()` to insert the particle pass. `ParticleRenderer` added as a member.
- **`renderer_deferred.cpp`**: Same as forward -- one new line in `draw()`, `ParticleRenderer` member.
- **Existing shaders**: Unchanged. Particle shader is standalone.
- **Existing components**: Unchanged.
- **Memory**: Each `ParticleComponent` allocates `m_max_particles * sizeof(Particle)` CPU-side (~64 bytes/particle = 64KB for 1000 particles) plus a stream VBO of `m_max_particles * 4 * sizeof(ParticleVertex)` (~48 bytes/vertex = 192KB for 1000 particles). A scene with 10 emitters at 1000 particles each uses ~2.5MB total.
- **Performance**: CPU-bound on particle simulation and vertex upload. At 10K total particles, expect <1ms simulation + <0.5ms upload on modern hardware. Draw calls are one per emitter per blend mode (not batched across emitters in v1). The single-vertex-per-particle approach with vertex shader expansion reduces upload to ~48KB per 1000 particles.

## Non-goals

- GPU compute particle simulation (SSBO + compute shader). This is a future optimization for emitters with 100K+ particles. The CPU pipeline established here defines the data model and rendering interface that a GPU backend would reuse.
- Particle collision with scene geometry (requires spatial queries or depth buffer readback).
- Sub-emitters (particles that spawn other particles on death). Can be layered on top of the emitter model later.
- Particle trails / ribbon rendering (requires connected strip geometry, not billboards).
- Lit particles (particles receiving scene lighting via the G-buffer or light probes). V1 particles are unlit / emissive only.
- Particle LOD or distance-based culling (future optimization).
- Sprite sheet animation on particles (subdividing the texture into frames over lifetime). Straightforward to add as additional fields on `ParticleComponent` later.
- Velocity-aligned particles (stretching billboards along velocity). Requires a shader variant; future enhancement.

## Development Workflow & Lessons Learned

### Key Discoveries During Implementation

#### 1. **Reflection & Serialization Pattern**
The engine uses `REFLECTED_STRUCT`, `REFLECTED_FIELD`, and `REFLECT_ENUM` macros for editor integration and YAML persistence. Only reflected fields are automatically serialized. Runtime state (GPU buffers, live arrays) must be manually initialized and are not persisted.

- **Pattern**: Separate configuration (reflected) from state (runtime)
- **Serialization**: Manual load/save in `Scene::create_entities_from_yaml()` and `Scene::save()`
- **Field naming**: Prefix all component fields with `m_` (e.g., `m_emission_rate`)

#### 2. **Random Number Generation**
The engine provides `Random::rfloat(min, max)` in `util/random_utils.h` for consistent, seeded random number generation. Use this instead of `std::rand()` for reproducible simulation.

#### 3. **Camera Access Pattern**
Access the primary camera via `scene->get_main_camera()` which returns `std::shared_ptr<Entity>`. The Entity contains a `CameraComponent` with methods like `get_position()` and `get_view()`. Do not try to access `g_runtime_context.m_camera` (does not exist).

**Correct pattern**:
```cpp
auto camera_entity = scene->get_main_camera();
if (camera_entity) {
    auto& camera_comp = camera_entity->get_component<CameraComponent>();
    glm::vec3 camera_pos = camera_comp.get_position();
}
```

#### 4. **Pipeline Blend Mode Configuration**
Blend modes are configured on `Pipeline::Description` using `src_blend_factor` and `dst_blend_factor` fields, not `blend_src`/`blend_dst`. The `blend` flag must be explicitly set to `true`.

**Correct pattern**:
```cpp
Pipeline::Description desc;
desc.blend = true;
desc.src_blend_factor = BlendFactor::SrcAlpha;
desc.dst_blend_factor = BlendFactor::One;  // Additive blend
desc.shader = shader;
m_pipeline = Pipeline::build(desc);
```

Available `BlendFactor` enums: `Zero, One, SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor, SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha`.

#### 5. **RenderGraph Integration**
Rendering passes are added via `RenderGraph::add_pass()` which returns a node builder. Passes can depend on input passes (e.g., forward transparency) via `set_passthrough()`. The execute lambda receives the render context and scene reference.

**Pattern**:
```cpp
rg.add_pass("particles")
    .set_passthrough("forward-transparency")
    .execute([this, scene](RenderGraphNode& node, GraphicsContext& ctx) {
        // Render code here
    });
```

#### 6. **System Update Ordering**
Systems are called from `Scene::on_update()` in a specific order:
1. `AnimationSystem::update()` - animation playback
2. **`ParticleSystem::update()`** - particle simulation (custom insertion point)
3. `ScriptSystem::update()` - user scripts (can read particle state)

This ordering ensures particles are simulated before scripts query their state.

#### 7. **Type Forward Declarations & Includes**
When a header file uses a type in a method signature or as a template parameter:
- If it's a pointer/reference to a struct/class: forward declare OR include the header
- If it's a value type or enum: must include the header (can't forward declare enums or types in template parameters)

**Example issue**: `ParticleRenderer::select_pipeline()` returns a `std::shared_ptr<Pipeline>` and uses `ParticleBlendMode` enum. The header must include both `render/pipeline.h` and `scene/component/particle.h`.

#### 8. **GIT-specific Build Issues (Windows)**
The Visual Studio 2026 build targets Debug x64 and uses Premake5 for project generation. When adding new files:
- Remember to regenerate projects via `dev/generate_vs2026.bat` if the `.vcxproj` files are out of sync
- Compilation errors often point to missing includes or incorrect API usage (not build configuration issues)
- The build system normalizes LF to CRLF on Windows; these warnings are harmless

### Common Compilation Mistakes & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `error C2061: syntax error: identifier 'X'` | Type `X` not declared in scope | Add `#include` for type definition |
| `error C2039: 'field': is not a member` | Wrong struct/class name or field name | Check struct name and use correct field names (e.g., `src_blend_factor` not `blend_src`) |
| `error C2065: 'identifier': undeclared` | Function/variable not declared | Add `#include` or forward declare if applicable |
| `error C3536: 'var': cannot be used before it is initialized` | Variable referenced before being assigned | Ensure assignment happens before use (e.g., after if check) |
| `error C2672: 'function': no matching overloaded function` | Wrong number of arguments | Check function signature (e.g., `glm::distance(a, b)` not `glm::distance(a)`) |

### Recommended Development Workflow

#### Phase 1: Planning & API Design
1. Read the proposal carefully. Understand the data structures, systems, and rendering integration points.
2. Identify all header files that need to be created or modified.
3. Create a mental map of dependencies (which files include which).

#### Phase 2: Component & System Implementation
1. Start with the **component** header and struct definition (with reflection macros).
2. Implement the **system** logic (CPU simulation, spawn, update, sort).
3. Test the component and system independently with simple debug output.

#### Phase 3: Rendering Integration
1. Create the **shader** file with correct @stage annotations.
2. Create the **renderer** class (pipelines, render graph pass).
3. Integrate into forward/deferred renderers as members.
4. Add render graph pass insertion in both `RendererForward` and `RendererDeferred`.

#### Phase 4: Scene Integration
1. Add the system update call in `Scene::on_update()`.
2. Add YAML loading in `Scene::create_entities_from_yaml()`.
3. Add YAML saving in `Scene::save()` (serializing all reflected fields).

#### Phase 5: Compilation & Testing
1. **Fix compilation errors systematically**:
   - Missing includes: Add `#include "path/to/header.h"` at the top of the file
   - Wrong API names: Search existing code for correct patterns (e.g., grep for "blend_factor" in renderer files)
   - Type mismatches: Ensure function signatures match (argument count, types)

2. **Standard test**:
   - `dev/build_vs2026.bat` - Full build (must succeed)
   - `engine/bin/Debug/game.exe --one-frame=10` - Executable test (should run without crashes)

3. **Validation**: If tests pass, the feature is integration-ready.

### Files to Consult as Reference

- **`AnimationSystem`** (`engine/runtime/source/scene/animation_system.h/cpp`) - Pattern for systems with ECS components
- **`Renderer2D`** (`engine/runtime/source/render/renderer/renderer_2d.h/cpp`) - Pattern for batched quad rendering
- **`SpriteComponent`** (`engine/runtime/source/scene/component/sprite.h`) - Pattern for reflected components with assets
- **`RenderGraph`** (`engine/runtime/source/render/render_graph.h`) - Pass insertion and execution
- **`CameraComponent`** (`engine/runtime/source/scene/component/camera.h`) - Camera access and transform patterns
- **`Scene::save()` and `Scene::create_entities_from_yaml()`** (`engine/runtime/source/scene/scene.cpp`) - YAML serialization patterns

### Openspec Best Practices

When writing an openspec proposal:
1. **Be explicit about APIs**: Include exact class names, field names, enum values, function signatures. Copy/paste from headers when possible.
2. **Show concrete examples**: Provide code snippets from existing systems that demonstrate the pattern.
3. **Document integration points**: Specify which files change, which systems call which, in what order.
4. **Call out non-goals early**: Clarify what is NOT being implemented to avoid scope creep.
5. **Include memory/performance estimates**: Help reviewers understand the resource footprint and optimization headroom.
6. **Use tables for configuration**: Configuration fields are easier to understand in tabular form with default values and editor hints.

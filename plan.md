# Implementation Plan - Lighting System

## Problem
The engine currently only supports a single hardcoded "Sun" directional light via global uniforms. We need to implement a proper lighting system with:
1.  `LightComponent` attached to entities (Directional, Point, Spot).
2.  Support for multiple lights in the renderer (forward rendering).
3.  Shadow mapping support (limited number of lights, likely 1 primary shadow caster).
4.  Editor visualization (gizmos) for lights.

## Proposed Changes

### 1. Component System
-   Create `runtime/source/scene/component/light.h`.
-   Define `LightType` enum (Directional, Point, Spot).
-   Define `LightComponent` with properties:
    -   `LightType type`
    -   `glm::vec3 color`
    -   `float intensity`
    -   `float range` (for Point/Spot)
    -   `float inner_cone` (for Spot)
    -   `float outer_cone` (for Spot)
    -   `bool cast_shadows`
-   Reflect the component using `REFLECTED_STRUCT`.

### 2. Shader System (GLSL)
-   Modify `engine/shader/common/lighting.glsl`:
    -   Define `Light` struct matching the C++ layout.
    -   Add `UniformBlock` for Lights (or append to Global, but separate is cleaner).
    -   Update `pbr_shading` or `phone_shading` to loop over active lights.
    -   Implement Point and Spot light attenuation and cone calculations.

### 3. Renderer (C++)
-   Modify `runtime/source/render/renderer/renderer_forward.h/cpp`:
    -   Add `std::shared_ptr<UniformBuffer> m_lights_buffer`.
    -   Define `LightConstants` struct to match shader layout.
    -   In `draw()`:
        -   Query all entities with `LightComponent`.
        -   Select the "primary" shadow caster (e.g., first directional light with shadows enabled).
        -   Populate `m_lights_buffer` with light data.
        -   Update `GlobalSettings` (Sun vars) to reflect the primary light (to maintain compatibility if needed, or just deprecate `sun_` vars in shader).
    -   Bind the new `Lights` uniform block to shaders.

### 4. Shadow Mapping
-   Update `RendererForward::draw` to use the primary shadow caster's transform for the shadow pass.
-   Currently, `RendererForward` calculates `sun_projview` based on `g->sun_direction`. We need to use the `LightComponent`'s transform (rotation) for direction and position.

### 5. Editor Gizmos
-   Modify `editor/source/main.cpp` (`EditorLayer`):
    -   In `on_update`, iterate over `LightComponent`s.
    -   Use `Renderer2D` to draw a billboard (quad) at the light's position.
    -   Use different colors/icons if possible.
-   Modify `runtime/source/render/renderer/renderer_2d.cpp`:
    -   Ensure `draw()` can handle `nullptr` scene (for manual quad flushing without scene sprites).

## Todos

### Phase 1: Component & Basic Rendering
-   [ ] Create `LightComponent` struct and header.
-   [ ] Add `LightComponent` to `Scene::create_entity` (optional, or just allow adding via Inspector if generic).
-   [ ] Create `lights.glsl` or update `lighting.glsl` with `Light` struct and uniforms.
-   [ ] Update `RendererForward` to manage `Lights` uniform buffer.
-   [ ] Update `RendererForward::draw` to collect lights and upload data.

### Phase 2: Shader Implementation
-   [ ] Implement Point Light logic in GLSL.
-   [ ] Implement Spot Light logic in GLSL.
-   [ ] Integrate multiple lights loop in `lighting.glsl`.

### Phase 3: Shadows
-   [ ] Logic to pick best shadow caster.
-   [ ] Update Shadow Pass to use picked light's matrix.

### Phase 4: Editor Integration
-   [ ] Fix `Renderer2D::draw` null check.
-   [ ] Add gizmo drawing loop in `EditorLayer`.

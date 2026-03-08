# Forward Renderer Refactor Plan

This document outlines the plan to refactor the `RendererForward` system in `z1engine`. The goal is to improve code readability, maintainability, and structure without altering existing functionalities.

## 1. Key Resources

-   [Coding Style](../AI_AGENT_CODING_STYLE.md) (**CRITICAL**: Read and follow strictly)
-   [Building Guide](../BUILDING.md)
-   [Contributing Guide](../CONTRIBUTING.md)

## 2. Important Guidelines

-   **Coding Style**:
    -   Use **Tabs** for indentation.
    -   Use **K&R** brace style.
    -   Types: `PascalCase`, Functions: `camelCase`, Members: `m_prefix`.
    -   Namespace: All code must be in `z1` namespace.
    -   Includes: `pch.h` must be first in cpp files.
-   **Minimal Changes**: Modification should be surgical. Avoid refactoring unrelated code.
-   **Attribution**: Add `Co-authored-by: Copilot <...>` to commit messages.

## 3. Current State

The `RendererForward` class (in `engine/runtime/source/render/renderer/renderer_forward.cpp`) contains a large `draw` method that handles:
1.  Resource management (history buffers).
2.  Camera and projection matrix calculations (including TAA jitter).
3.  Light data collection and upload.
4.  Cascade Shadow Map (CSM) split calculations.
5.  Render graph construction and execution.
6.  Inline definitions for Shadow, Main, Velocity, TAA, and Post-processing passes.

This makes the `draw` method difficult to read and maintain.

## 4. Refactoring Goals

1.  **Decompose `draw` method**: Break down the monolithic `draw` method into smaller, focused methods.
2.  **Encapsulate Render Pass Setup**: Create dedicated methods for adding each render pass to the `RenderGraph`.
3.  **Extract Logic**: Move light update and CSM calculation logic to separate helper methods.
4.  **Preserve Functionality**: Ensure no logic or behavior is changed during the refactor.

## 5. Proposed Changes

### 5.1 Extract Logic Helpers

Create private helper methods in `RendererForward` class:

-   `void update_lights(std::shared_ptr<Scene> const& scene);`
    -   Extracts the light collection and `m_lights_buffer` update logic.
-   `void calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir);`
    -   Extracts the CSM split calculation and `g->sun_projview` update logic.

### 5.2 Extract Render Pass Helpers

Create private helper methods to add passes to the `RenderGraph`:

-   `void add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene);`
-   `void add_main_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);`
-   `void add_velocity_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);`
-   `void add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_buffer);`
-   `void add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target, std::shared_ptr<Framebuffer> const& source);`

### 5.3 Refactor `draw` Method

The `draw` method will be simplified to:

```cpp
void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
    PROFILE_FUNCTION();

    // 1. Update Resources (History Buffers, Camera)
    update_resources(framebuffer); 
    
    // 2. Update Scene Data (Lights, CSM)
    update_scene_data(scene);

    // 3. Build Render Graph
    RenderGraph rg;
    add_shadow_pass(rg, scene);
    add_main_pass(rg, scene, framebuffer);
    add_velocity_pass(rg, scene, framebuffer);
    add_taa_pass(rg, ...);
    add_postprocess_pass(rg, framebuffer, ...);

    // 4. Execute
    rg.compile();
    rg.execute();
}
```

## 6. Detailed Steps

1.  **Modify `RendererForward.h`**: Add declarations for new private methods.
2.  **Modify `RendererForward.cpp`**:
    -   Implement `update_lights`.
    -   Implement `calculate_csm_splits`.
    -   Implement pass addition methods, moving the lambda logic from `draw`.
    -   Rewrite `draw` to use these new methods.
3.  **Verify**: Ensure the code compiles and the logic flow remains identical.

## 7. Build Verification

Before marking a task as complete, ensure the project compiles successfully.

**Steps:**
1.  Run the generation script (if you added/removed files):
    ```cmd
    dev\generate_vs2026.bat
    ```
2.  Run the compile script:
    ```cmd
    dev\compile_vs2026.bat
    ```
    *(Note: This script assumes Visual Studio 2026. Modify the path in the script if necessary.)*
3.  Format code using python script:
    ```cmd
    python utils\format_code.py
    ```

## 8. Shader Validation

**Status**: N/A (No shader changes expected)

If the task involves modifying or creating shaders (glsl), you **MUST** validate them using the `shader_validator` tool.

**Steps:**
1.  Build the solution (this builds `shader_validator`).
2.  Run the validator against your shader file:
    ```cmd
    engine\bin\test\Release\shader_validator.exe path\to\your\shader.glsl
    ```

## 9. Testing

-   Run existing tests if relevant:
    ```cmd
    engine\bin\test\Release\test_math.exe
    ```
-   Create new tests in `engine/test/` if adding new functionality.

## 10. Considerations

-   **State Management**: Some variables (like `projview`, `view`, `transform`) are used across multiple passes. These might need to be passed as arguments or stored as member variables during the frame (though `RendererForward` seems stateless per frame except for history).
-   **RenderGraph Dependencies**: Ensure the strings used for pass names and resource names ("scene-color", "velocity", etc.) match exactly to maintain graph connectivity.

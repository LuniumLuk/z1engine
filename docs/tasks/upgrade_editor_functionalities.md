# Task: Upgrade Editor Functionalities

This task involves enhancing the Editor's usability and feature set, specifically focusing on asset management and interaction.

## 1. Key Resources

-   **Codebase**: `engine/editor/source/`, `engine/runtime/source/asset/`
-   **Key Files**:
    -   `engine/editor/source/browser.h`: Content Browser UI and logic.
    -   `engine/editor/source/main.cpp`: Editor layout, Inspector (properties) panel.
    -   `engine/runtime/source/asset/asset_manager.cpp`: Asset management logic (move, remove).

## 2. Requirements

### 2.1 Asset Rename
-   **Goal**: Allow users to rename assets directly in the Content Browser.
-   **Implementation Details**:
    -   Add a "Rename" option to the asset context menu in `ContentBrowser` (`browser.h`).
    -   Use `AssetManager::move_asset` to handle the file renaming on disk.
    -   Ensure the UI updates immediately.

### 2.2 Create New Asset
-   **Goal**: Allow creation of new assets (specifically Materials) from the browser.
-   **Implementation Details**:
    -   Add a "Create" submenu in the Content Browser context menu (right-click on folder background).
    -   Support creating a **Material Instance** (requires selecting a parent Material/Shader or creating a default one).
    -   Ideally support creating empty **Shaders** or **Scripts** templates.

### 2.3 Drag-Drop Material Instance
-   **Goal**: Enable dragging a Material Instance from the Content Browser and dropping it onto a Mesh primitive slot in the Inspector.
-   **Implementation Details**:
    -   Locate `show_properties` in `engine/editor/source/main.cpp`.
    -   Find the `StaticMeshComponent` and `SkeletalMeshComponent` sections.
    -   Add `accept_payload("ASSET_ITEM", ...)` logic to the material slots (similar to how `SpriteComponent` handles texture drops).
    -   Update the mesh primitive's material assignment upon drop.

### 2.4 Safe Delete Asset
-   **Goal**: Prevent accidental deletion of assets that are currently referenced by other assets or entities.
-   **Implementation Details**:
    -   Implement a reference check mechanism in `AssetManager`. This likely requires scanning loaded assets or maintaining a dependency graph.
    -   Update `remove_asset` to return a failure/warning if references exist.
    -   Update the "Delete asset" popup in `ContentBrowser` to:
        1.  Check for references first.
        2.  Warn the user if references exist.
        3.  Allow "Force Delete" if the user insists (potentially leaving broken references).

### 2.5 Import Asset Dialog
-   **Goal**: Allow users to customize the destination path/name when importing assets.
-   **Implementation Details**:
    -   Modify the "Import asset" flow in `ContentBrowser`.
    -   Instead of immediately importing after `open_file_dialog`, open a **Modal Popup**.
    -   The popup should show:
        -   Selected source file.
        -   Editable destination folder/filename (defaulting to current folder).
        -   "Import" and "Cancel" buttons.
    -   Call `import_asset` with the user-specified path upon confirmation.

## 3. Build Verification

-   Run `dev\generate_vs2026.bat` if you add new files.
-   Run `dev\compile_vs2026.bat` to verify compilation.
-   Run the editor and test each functionality manually:
    -   Rename an asset and check if it persists.
    -   Create a new material.
    -   Drag-drop a material onto a mesh.
    -   Try to delete a referenced asset (should warn).
    -   Import an asset and change its target path.

## 4. Implementation Notes

-   **Refactoring**: `engine/editor/source/main.cpp` is getting large. Consider extracting the Inspector/Properties panel into a separate class/file (e.g., `inspector.h/cpp`) if it makes the changes cleaner, though not strictly required for this task.
-   **Reference Counting**: Implementing a full dependency graph might be complex. A simple scan of *loaded* assets might be a sufficient first step for "Safe Delete".

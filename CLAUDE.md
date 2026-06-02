# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
# From repo root
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j8

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test
ctest --test-dir build -R AnimData --output-on-failure
```

## Architecture

This is a Cocos2d-x 4.x animation editor — a standalone C++17 desktop app using Dear ImGui (docking branch) + GLFW for the UI and OpenGL 3.3 Core Profile for rendering. The repo is a CMake workspace with three sub-projects:

### Top-level `CMakeLists.txt`
- Builds `third_party/glfw` from source via `add_subdirectory`
- ImGui sources are collected by `AnimEditor/CMakeLists.txt` directly from `third_party/imgui`
- Registers CTest via `enable_testing()`

### `AnimEditor/` — The editor application
- **`src/main.cpp`** — GLFW window + ImGui initialization, Docking setup (`DockSpaceOverViewport` + `DockBuilder`). The dock layout: Files (left sidebar), Preview (center), Properties (right sidebar), Nodes (bottom-left), Timeline (bottom-right).
- **`src/core/`** — Data model and engine
  - `AnimData.h` — Core data structures: `Node` (tree with `NodeProperties`), `Track` (property animation with `Keyframe` list), `Animation` (named clip with duration + tracks), `AnimProject` (node tree + animation clips + events). Uses `std::variant<float, int, bool, std::string>` for keyframe values.
  - `SceneGraph` — Mutable tree of `NodePtr` with add/remove/find/reparent/rename/reorder operations. Owns the editor-side node hierarchy.
  - `Easing` — Easing function evaluator for keyframe interpolation.
  - `AnimationEngine` — Evaluates all tracks at a given time, producing per-node property values.
  - `Serializer` — JSON save/load of `AnimProject` via nlohmann/json.
  - `UndoSystem` — Generic undo/redo stack for editor operations.
- **`src/ui/`** — ImGui panels, each with a `render()` method
  - `EditorUI` — Coordinator: owns all panels, wires callbacks between them (e.g., node selection → timeline + property panel updates), manages `AnimProject` lifecycle.
  - `NodeTreePanel` — Tree view of scene nodes. Add/delete/reparent via context menu.
  - `PropertyPanel` — Edit properties of the selected node (position, scale, rotation, opacity, etc.).
  - `TimelinePanel` — Clip selector combo + transport controls + adaptive time ruler + track rows with keyframe diamonds. Shows "potential tracks" for un-animated properties of the selected node — double-click to create first keyframe.
  - `PreviewCanvas` — Displays the FBO texture from `CocosEmbed` via `ImGui::Image`.
  - `FileBrowserPanel` — Basic file browser for `.anim` project files.
- **`src/renderer/`**
  - `CocosEmbed` — Creates an FBO with color texture + depth renderbuffer. Renders a test pattern (checkerboard + crosshair + border) via CPU pixel generation + `glTexSubImage2D`. Eventually this will render a Cocos2d-x scene into the FBO.
- **`tests/`** — GoogleTest unit tests for each core module.

### `AnimRuntime/` — Runtime animation library (static lib)
- `AnimData.h` — Mirror of the editor's data structures, for runtime-only use.
- `AnimLoader` — Load `.anim` JSON files.
- `AnimPlayer` — Play back animation clips, evaluating tracks against a target node tree.
- `Easing` — Same easing functions as the editor.
- Will link against `cocos2d` once the engine is integrated.

### `third_party/`
- `imgui/` — Dear ImGui docking branch (source files compiled directly into the editor target)
- `glfw/` — GLFW 3.x built from source
- `json/include/` — nlohmann/json single-header

## Key patterns

- **Callback wiring**: Panels don't know about each other. `EditorUI::init()` wires `std::function` callbacks: node selection propagates to both `PropertyPanel` and `TimelinePanel`; timeline keyframe add/remove callbacks mutate the `AnimProject` through `EditorUI`.
- **Data ownership**: `EditorUI` owns the `AnimProject` (shared_ptr), `SceneGraph` mirrors the node tree for editor operations. Panels receive raw pointers or references; they don't own data.
- **Animation lookup**: Always use `TimelinePanel::getCurrentAnimationName()` to find the active clip — never assume the first animation.
- **ImGui child windows for tracks**: Each track row and potential track uses `BeginChild` with a unique ID (`##track_nodeId_property` or `##pot_nodeId_property`). This scopes the ID stack so invisible buttons inside different tracks don't collide.
- **FBO rendering**: `CocosEmbed` renders to an off-screen FBO. `PreviewCanvas` displays the FBO's color texture via `ImGui::Image` with a UV flip (`ImVec2(0,1)` to `ImVec2(1,0)`) because OpenGL and ImGui have inverted Y axes.

## Dependencies

- **macOS**: `brew install glfw googletest` (GLFW also built from source via `third_party/glfw`)
- OpenGL 3.3 Core Profile (no extensions needed)
- Everything else (ImGui, nlohmann/json) is vendored in `third_party/`

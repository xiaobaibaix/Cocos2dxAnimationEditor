# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Rules (MUST FOLLOW)

1. **Stay in your module** — never edit files owned by other modules. Check [docs/MODULES.md](docs/MODULES.md) for ownership.
2. **Debug panel required** — every feature needs a debug panel (floating window via `Debug >` menu) for independent visual testing.
3. **Build + test before commit** — `cmake --build build -j8 && ctest --test-dir build --output-on-failure`.
4. **No cross-module merges** — merge only to develop, never to another module branch.
5. **Self-contained testing** — debug panels inject their own test data, don't rely on other modules.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j8
ctest --test-dir build --output-on-failure
ctest --test-dir build -R AnimData --output-on-failure   # single test
```

## Branch Model

```
master                     ← stable releases only
  └─ develop               ← integration + debug framework
       ├─ feature/core     ← data layer (AnimData, SceneGraph, Serializer, UndoSystem)
       ├─ feature/files    ← file browser, native dialogs, recent files
       ├─ feature/nodetree ← node tree panel, property panel
       ├─ feature/timeline ← timeline panel, keyframes, event frames
       ├─ feature/preview  ← preview canvas, CocosEmbed FBO rendering
       └─ feature/animation← AnimationEngine, AnimRuntime (AnimPlayer/AnimLoader)
```

**Merge flow**: sub-feature → module branch → develop → (stable) → master.
Never merge between module branches; always go through develop.

## Architecture

C++17 desktop app: Dear ImGui (docking) + GLFW + OpenGL 3.3 Core Profile on macOS.

### Source layout

```
AnimEditor/src/
  main.cpp          — GLFW window, ImGui init, docking layout (5 windows)
  core/             — AnimData, SceneGraph, Serializer, UndoSystem, Easing, AnimationEngine
  ui/               — EditorUI (coordinator) + 5 panels: FileBrowser, NodeTree, Property, Timeline, PreviewCanvas
  renderer/         — CocosEmbed (FBO + OpenGL test pattern → texture)
  debug/            — Debug framework (DebugHost + DebugPanelBase + DemoDebugPanel)
  platform/         — NativeDialogs (.mm for macOS Cocoa)
  tests/            — GoogleTest unit tests
AnimRuntime/        — Static lib: AnimPlayer, AnimLoader, Easing (runtime)
third_party/        — imgui (docking), glfw, nlohmann/json
```

### Key patterns

- **Callback wiring**: EditorUI::init() wires `std::function` callbacks between panels. Panels don't know about each other.
- **Data ownership**: EditorUI owns `AnimProject` (shared_ptr) and `SceneGraph`. Panels get raw pointers.
- **Animation lookup**: Always use `TimelinePanel::getCurrentAnimationName()` — never assume first animation.
- **ImGui child windows**: Each track row uses `BeginChild` with unique ID (`##track_nodeId_property`) to avoid ID stack collisions.
- **FBO Y-flip**: PreviewCanvas displays FBO texture with UV flip (`(0,1)→(1,0)`) because OpenGL/ImGui Y axes are inverted.

### Dependencies

```bash
brew install glfw googletest
```
OpenGL 3.3 Core Profile (no extensions). Everything else vendored in `third_party/`.

## Debug Framework

Module branches use floating debug windows to test independently of other modules.

### Adding a debug panel

1. Create a class inheriting `DebugPanelBase` (override `name()` and `render()`)
2. Register in `EditorUI::init()` via `registerDebugPanel<YourPanel>()`

The panel appears under the Debug menu as a toggleable floating window. Debug panels must be self-contained — inject test data, don't depend on other modules' state.

See `src/debug/DemoDebugPanel.h` for a minimal example.

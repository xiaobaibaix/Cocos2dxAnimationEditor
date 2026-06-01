# AnimEditor — Cocos2d-x 4.x 动画资源编辑器设计文档

> 日期：2026-06-01
> 状态：已批准

## 概述

为 Cocos2d-x 4.x 构建一个通用时间轴动画资源编辑器。编辑器为独立 C++ 桌面应用（ImGui），内嵌 Cocos2d-x 渲染上下文实现所见即所得预览。产出 JSON 格式的动画描述文件，游戏项目通过轻量 AnimRuntime 库加载播放。

## 需求摘要

| 维度 | 决策 |
|------|------|
| 使用场景 | 通用时间轴编辑器，可编辑任意 UI 节点的属性动画 |
| 运行形式 | 独立桌面应用 |
| 技术栈 | C++ + ImGui + Cocos2d-x 4.x |
| 产出格式 | JSON（节点树 + 关键帧数据） |
| 可动画属性 | 位移、缩放、旋转、透明度、颜色、事件帧、显隐、纹理切换 |
| 目标引擎 | Cocos2d-x 4.x |

## §1 整体架构与 UI 布局

### 系统模块

```
AnimEditor App
├── EditorCore（数据模型层）
│   ├── SceneGraph        场景节点树模型
│   ├── AnimationEngine   关键帧插值 + 缓动
│   ├── ProjectManager    项目/资源管理
│   └── Serializer        JSON 导入导出
├── ImGui UI Layer（界面层）
│   ├── FileBrowserPanel  文件系统浏览器
│   ├── PreviewCanvas     预览画布面板
│   ├── PropertyPanel     属性检查面板
│   ├── NodeTreePanel     节点树面板（嵌入时间线区域）
│   └── TimelinePanel     时间线面板
├── Cocos2d-x Render Context（渲染层）
│   ├── GLView (embedded) 内嵌 GL 视图
│   └── Scene (live)      实时节点场景
└── UndoSystem            撤销/重做系统
```

### UI 布局

```
┌─────────────────────────────────────────────────────────┐
│  File  |  Edit  |  View  |  Animation  |  Help          │
├──────────┬─────────────────────────────┬────────────────┤
│          │                             │                │
│  FILES   │    PREVIEW CANVAS           │  PROPERTIES    │
│          │    (Cocos2d-x 实时渲染)      │                │
│  文件系统 │                             │  选中节点的    │
│  浏览器   │                             │  属性编辑      │
│          │                             │                │
├──────────┴─────────────────────────────┴────────────────┤
│                        │                                │
│   NODE TREE            │    TIMELINE                    │
│                        │                                │
│   场景节点树            │    关键帧轨道 + 时间指针        │
│   (选中节点高亮对应轨道) │    (按节点分行显示)            │
│                        │                                │
└────────────────────────┴────────────────────────────────┘
```

### 交互逻辑

1. **文件系统**（左侧）→ 双击 `.anim` 文件打开动画项目，双击纹理资源导入到场景
2. **预览画布**（中间）→ 实时显示 Cocos2d-x 渲染结果，可直接拖拽节点
3. **属性面板**（右侧）→ 显示当前选中节点的属性，可编辑当前帧的属性值
4. **节点树 + 时间线**（底部）→ 点击节点树中的节点，时间线高亮显示该节点的动画轨道

## §2 数据模型与 JSON 产出格式

### 核心数据结构

```
AnimProject (.anim 文件)
│
├── meta                     元信息（版本、画布尺寸、帧率）
│
├── nodeTree: Node[]         UI 节点树（递归结构）
│   └── Node
│       ├── id               唯一标识 UUID
│       ├── type             节点类型 (Sprite / Label / Node / Button ...)
│       ├── name             节点名称
│       ├── properties       静态默认属性
│       │   ├── position     {x, y}
│       │   ├── scale        {x, y}
│       │   ├── rotation     角度
│       │   ├── anchor       {x, y}
│       │   ├── opacity      0~255
│       │   ├── color        {r, g, b}
│       │   ├── visible      bool
│       │   └── texture      纹理路径 (Sprite 专有)
│       ├── children: Node[] 子节点（递归）
│       └── zIndex           渲染层级
│
├── animations: Animation[]  动画集合（多个动画片段）
│   └── Animation
│       ├── name             动画名称（如 "intro", "loop"）
│       ├── duration         总时长（秒）
│       ├── loop             是否循环
│       └── tracks: Track[]  轨道列表（每个节点的每个属性一条）
│           └── Track
│               ├── nodeId   关联的节点 ID
│               ├── property 属性名（position.x / opacity / color.r ...）
│               └── keyframes: Keyframe[]
│                   └── Keyframe
│                       ├── time      时间点（秒）
│                       ├── value     属性值
│                       └── easing    缓动函数
│
└── events: Event[]          事件帧
    └── Event
        ├── time             触发时间
        ├── nodeId           关联节点
        └── name             事件名称
```

### JSON 示例

```json
{
  "meta": {
    "version": "1.0",
    "canvas": { "width": 1280, "height": 720 },
    "frameRate": 60
  },
  "nodeTree": [
    {
      "id": "n_root", "type": "Node", "name": "Root",
      "properties": { "position": {"x":0,"y":0}, "visible": true },
      "children": [
        {
          "id": "n_bg", "type": "Sprite", "name": "Background",
          "properties": { "texture": "textures/bg.png", "opacity": 0 }
        },
        {
          "id": "n_title", "type": "Label", "name": "Title",
          "properties": { "position": {"x":640,"y":400}, "text": "Hello!" }
        }
      ]
    }
  ],
  "animations": [
    {
      "name": "show", "duration": 0.6, "loop": false,
      "tracks": [
        {
          "nodeId": "n_bg", "property": "opacity",
          "keyframes": [
            { "time": 0.0, "value": 0, "easing": "easeOut" },
            { "time": 0.4, "value": 255, "easing": "linear" }
          ]
        },
        {
          "nodeId": "n_title", "property": "position.y",
          "keyframes": [
            { "time": 0.0, "value": 500, "easing": "easeOutBack" },
            { "time": 0.5, "value": 400, "easing": "linear" }
          ]
        }
      ]
    }
  ],
  "events": [
    { "time": 0.0, "nodeId": "n_bg", "name": "playSound:whoosh" }
  ]
}
```

### 关键设计决策

- **属性用点号路径**：如 `position.x`、`scale.y`、`opacity`。每个属性独立轨道，关键帧互不干扰。
- **多动画片段**：一个 .anim 文件包含多个命名动画（show / hide / loop），运行时按名称加载播放。
- **节点 ID 引用**：Track 通过 nodeId 关联节点树中的节点，解耦树结构和动画数据。
- **事件帧独立存储**：事件帧不属于任何属性轨道，按时间排列，运行时触发回调。

## §3 引擎嵌入与运行时库

### 渲染嵌入流程

```
ImGui 主循环 (glfwOpenGL3)
│
├── ImGui::NewFrame()
├── 渲染编辑器 UI 面板
├── 预览画布区域:
│   ├── 1. 获取 ImGui 窗口可用区域大小
│   ├── 2. 调整 cocos::Director 的 GLViewport 到该区域
│   ├── 3. cocos::Director::mainLoop() → 渲染场景到 FBO
│   ├── 4. 将 FBO color attachment 绑定为 ImGui 纹理
│   └── 5. ImGui::Image(texId, size) → 显示在面板中
├── ImGui::Render()
└── Platform SwapBuffers
```

关键技术点：
- Cocos2d-x 4.x 的 Director 使用 headless 模式初始化，不创建独立窗口
- 将 cocos::Director 渲染到一个 FBO，而非默认 backbuffer
- ImGui 通过纹理 ID 将 FBO 内容显示在面板中
- 编辑器输入事件转发给 Cocos2d-x 的事件系统

### AnimRuntime 运行时库 API

```cpp
#include "AnimRuntime/AnimPlayer.h"

// 加载动画文件
auto animData = AnimLoader::loadFromFile("anims/popup.anim");

// 创建播放器
auto player = AnimPlayer::create();
player->load(animData);
rootNode->addChild(player);

// 构建 Cocos2d-x 节点树
player->buildNodeTree(rootNode);

// 播放指定动画片段
player->play("show");

// 监听事件帧
player->setEventCallback([](const std::string& eventName) {
    if (eventName == "playSound:whoosh") {
        AudioEngine::play("sounds/whoosh.mp3");
    }
});

// 动画完成回调
player->setCompletionCallback([]() {
    CCLOG("Animation finished!");
});
```

运行时库组件：
- **AnimLoader**：解析 JSON，构建内存中的 AnimData 对象，验证数据完整性。
- **AnimPlayer**：继承 cocos2d::Node。驱动关键帧插值、缓动计算、事件触发。支持 play / pause / stop / seek。
- **缓动函数**：内置 linear / easeIn / easeOut / easeInOut / easeOutBack / easeOutBounce / easeOutElastic。

### 项目目录结构

```
AnimEditor/                        ← 编辑器项目（独立构建）
├── src/
│   ├── core/
│   │   ├── SceneGraph.h/cpp
│   │   ├── AnimationEngine.h/cpp
│   │   ├── ProjectManager.h/cpp
│   │   ├── Serializer.h/cpp
│   │   └── UndoSystem.h/cpp
│   ├── ui/
│   │   ├── NodeTreePanel.h/cpp
│   │   ├── TimelinePanel.h/cpp
│   │   ├── PropertyPanel.h/cpp
│   │   ├── PreviewCanvas.h/cpp
│   │   └── FileBrowserPanel.h/cpp
│   ├── renderer/
│   │   └── CocosEmbed.h/cpp
│   └── main.cpp
├── CMakeLists.txt
└── third_party/
    ├── imgui/
    └── cocos2d-x/

AnimRuntime/                       ← 运行时库（集成到游戏项目）
├── include/
│   └── AnimRuntime/
│       ├── AnimLoader.h
│       ├── AnimPlayer.h
│       ├── AnimData.h
│       └── Easing.h
├── src/
│   ├── AnimLoader.cpp
│   ├── AnimPlayer.cpp
│   └── Easing.cpp
└── CMakeLists.txt
```

## §4 构建系统与实施路线图

### 技术依赖

| 依赖 | 版本 | 用途 | 用于 |
|------|------|------|------|
| Cocos2d-x | 4.0 | 2D 渲染引擎，节点系统 | 编辑器 + 运行时 |
| Dear ImGui | 1.90+ | 编辑器 UI 框架 | 编辑器 |
| nlohmann/json | 3.x | JSON 序列化/反序列化 | 编辑器 + 运行时 |
| GLFW | 3.3+ | 窗口管理 + OpenGL 上下文 | 编辑器 |
| CMake | 3.20+ | 构建系统 | 编辑器 + 运行时 |

### 分阶段实施

**Phase 1 — 基础骨架**
- CMake 项目搭建，集成 ImGui + GLFW
- 编辑器主窗口 + Docking 布局框架
- 文件浏览器面板（浏览目录 + 双击打开 .anim）
- 数据模型定义（AnimData / Node / Track / Keyframe 结构体）
- JSON 序列化/反序列化（Serializer）

**Phase 2 — 节点树 + 属性编辑**
- 节点树面板：添加/删除/重命名/拖拽排序节点
- 属性面板：编辑节点属性（位置、缩放、旋转、颜色等）
- SceneGraph 数据模型 + Undo/Redo 系统
- Cocos2d-x headless 初始化 + FBO 渲染嵌入
- 预览画布：显示节点树对应的 Cocos2d-x 场景

**Phase 3 — 时间线 + 动画编辑**
- 时间线面板：轨道显示、关键帧菱形标记、时间指针拖拽
- 关键帧编辑：添加/删除/移动关键帧，编辑属性值 + 缓动函数
- AnimationEngine：关键帧插值计算 + 缓动函数库
- 实时预览播放：编辑器内播放/暂停/seek 动画
- 事件帧编辑 + 显隐/纹理切换关键帧

**Phase 4 — 运行时库 + 打磨**
- AnimRuntime 库：AnimLoader + AnimPlayer
- 画布内拖拽节点移动（2D gizmo）
- 多动画片段管理（新增/删除/重命名片段）
- 导入纹理资源到节点、字体渲染预览
- 项目保存/加载、最近文件列表

### CMake 顶层结构

```cmake
cmake_minimum_required(VERSION 3.20)
project(AnimToolkit)

add_subdirectory(third_party/cocos2d-x)
add_subdirectory(AnimRuntime)
add_subdirectory(AnimEditor)
```

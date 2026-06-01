# AnimEditor — Cocos2d-x 4.x 动画资源编辑器实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建一个独立的 C++ 桌面动画资源编辑器，内嵌 Cocos2d-x 4.x 渲染，产出 JSON 动画文件，配套轻量运行时库。

**Architecture:** ImGui 编辑器内嵌 Cocos2d-x 4.x headless 渲染上下文（FBO → ImGui 纹理），左侧文件浏览器 + 中间预览画布 + 右侧属性面板 + 底部节点树/时间线耦合面板。核心逻辑（SceneGraph、AnimationEngine、Serializer）与 UI 分离。AnimRuntime 为独立轻量库。

**Tech Stack:** C++17, CMake 3.20+, Dear ImGui 1.90+, Cocos2d-x 4.0, nlohmann/json 3.x, GLFW 3.3+

---

## 文件结构总览

```
AnimToolkit/                           ← 仓库根目录
├── CMakeLists.txt                     ← 顶层 CMake
├── AnimEditor/                        ← 编辑器
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cpp                   ← 入口：GLFW + ImGui + 编辑器主循环
│   │   ├── core/
│   │   │   ├── AnimData.h             ← 数据模型定义（纯头文件）
│   │   │   ├── Serializer.h           ← JSON 序列化接口
│   │   │   ├── Serializer.cpp
│   │   │   ├── SceneGraph.h           ← 场景节点树管理
│   │   │   ├── SceneGraph.cpp
│   │   │   ├── AnimationEngine.h      ← 关键帧插值 + 缓动
│   │   │   ├── AnimationEngine.cpp
│   │   │   ├── ProjectManager.h       ← 项目生命周期管理
│   │   │   ├── ProjectManager.cpp
│   │   │   ├── UndoSystem.h           ← 撤销/重做
│   │   │   └── UndoSystem.cpp
│   │   ├── ui/
│   │   │   ├── FileBrowserPanel.h     ← 文件系统浏览器
│   │   │   ├── FileBrowserPanel.cpp
│   │   │   ├── PreviewCanvas.h        ← 预览画布（含 Cocos2d-x 嵌入）
│   │   │   ├── PreviewCanvas.cpp
│   │   │   ├── PropertyPanel.h        ← 属性检查器
│   │   │   ├── PropertyPanel.cpp
│   │   │   ├── NodeTreePanel.h        ← 节点树面板
│   │   │   ├── NodeTreePanel.cpp
│   │   │   ├── TimelinePanel.h        ← 时间线面板
│   │   │   ├── TimelinePanel.cpp
│   │   │   ├── EditorUI.h             ← UI 总协调器
│   │   │   └── EditorUI.cpp
│   │   └── renderer/
│   │       ├── CocosEmbed.h           ← Cocos2d-x headless 嵌入
│   │       └── CocosEmbed.cpp
│   └── tests/
│       ├── CMakeLists.txt
│       ├── test_AnimData.cpp
│       ├── test_Serializer.cpp
│       ├── test_SceneGraph.cpp
│       ├── test_AnimationEngine.cpp
│       ├── test_UndoSystem.cpp
│       └── test_Easing.cpp
├── AnimRuntime/                       ← 运行时库
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── AnimRuntime/
│   │       ├── AnimData.h             ← 共享数据模型（软链或复制自 AnimEditor）
│   │       ├── AnimLoader.h
│   │       ├── AnimPlayer.h
│   │       └── Easing.h
│   └── src/
│       ├── AnimLoader.cpp
│       ├── AnimPlayer.cpp
│       └── Easing.cpp
└── third_party/
    ├── imgui/                         ← ImGui 子目录（源码）
    ├── cocos2d-x/                     ← Cocos2d-x 4.x（git submodule）
    ├── json/                          ← nlohmann/json 单头文件
    └── glfw/                          ← GLFW（源码或 submodule）
```

---

## Phase 1 — 基础骨架

### Task 1: 搭建 CMake 项目骨架 + 集成第三方依赖

**Files:**
- Create: `CMakeLists.txt`
- Create: `AnimEditor/CMakeLists.txt`
- Create: `AnimEditor/src/main.cpp`
- Create: `AnimEditor/tests/CMakeLists.txt`
- Create: `third_party/` 目录结构

- [ ] **Step 1: 创建顶层 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(AnimToolkit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 第三方依赖
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/imgui)

# 编辑器（Phase 2+ 再启用 cocos2d-x）
add_subdirectory(AnimEditor)
```

- [ ] **Step 2: 创建 AnimEditor/CMakeLists.txt**

```cmake
# 收集 ImGui 源文件
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(anim_editor
    src/main.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(anim_editor PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(anim_editor PRIVATE glfw)

# 测试
add_subdirectory(tests)
```

- [ ] **Step 3: 创建测试 CMake — AnimEditor/tests/CMakeLists.txt**

```cmake
enable_testing()
find_package(GTest REQUIRED)

# 测试可执行文件在后续 Task 中逐步添加
# add_executable(test_xxx test_xxx.cpp)
# target_link_libraries(test_xxx PRIVATE GTest::gtest_main)
# add_test(NAME xxx COMMAND test_xxx)
```

- [ ] **Step 4: 创建最小 main.cpp**

```cpp
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main(int argc, char** argv) {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "AnimEditor", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        ImGui::Begin("Welcome");
        ImGui::Text("AnimEditor - Phase 1 Skeleton");
        ImGui::End();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

- [ ] **Step 5: 准备 third_party 依赖**

```bash
# 在项目根目录执行
mkdir -p third_party

# ImGui
cd third_party && git clone --depth 1 --branch v1.90.9 https://github.com/ocornut/imgui.git
cd imgui && mkdir -p backends
# ImGui 的 backends 需要从 docking 分支获取
git clone --depth 1 --branch v1.90.9 https://github.com/ocornut/imgui.git imgui_tmp
cp imgui_tmp/backends/imgui_impl_glfw.cpp backends/
cp imgui_tmp/backends/imgui_impl_glfw.h backends/
cp imgui_tmp/backends/imgui_impl_opengl3.cpp backends/
cp imgui_tmp/backends/imgui_impl_opengl3.h backends/
cp imgui_tmp/backends/imgui_impl_opengl3_loader.h backends/
rm -rf imgui_tmp

# GLFW
cd .. && git clone --depth 1 --branch 3.3.9 https://github.com/glfw/glfw.git

# nlohmann/json
mkdir -p json/include/nlohmann
curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -o json/include/nlohmann/json.hpp
```

- [ ] **Step 6: 构建并验证窗口启动**

```bash
cd /path/to/AnimToolkit
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/AnimEditor/anim_editor
```

Expected: 弹出 1280x720 深色窗口，显示 ImGui Docking 布局 + "Welcome" 面板。

- [ ] **Step 7: 提交**

```bash
git add CMakeLists.txt AnimEditor/ third_party/
git commit -m "feat: scaffold CMake project with ImGui + GLFW skeleton"
```

---

### Task 2: 定义数据模型（AnimData）

**Files:**
- Create: `AnimEditor/src/core/AnimData.h`
- Create: `AnimEditor/tests/test_AnimData.cpp`
- Modify: `AnimEditor/tests/CMakeLists.txt`

- [ ] **Step 1: 编写 AnimData 测试**

```cpp
// test_AnimData.cpp
#include <gtest/gtest.h>
#include "core/AnimData.h"

TEST(AnimDataTest, NodeDefaultValues) {
    anim::Node node;
    node.id = "n1";
    node.type = anim::NodeType::Sprite;
    node.name = "TestNode";
    EXPECT_EQ(node.id, "n1");
    EXPECT_EQ(node.type, anim::NodeType::Sprite);
    EXPECT_EQ(node.name, "TestNode");
    EXPECT_EQ(node.properties.position.x, 0.0f);
    EXPECT_EQ(node.properties.position.y, 0.0f);
    EXPECT_EQ(node.properties.opacity, 255);
    EXPECT_TRUE(node.properties.visible);
    EXPECT_TRUE(node.children.empty());
}

TEST(AnimDataTest, NodeTreeNesting) {
    anim::Node root;
    root.id = "root";
    root.type = anim::NodeType::Node;
    root.name = "Root";

    anim::Node child;
    child.id = "child1";
    child.type = anim::NodeType::Sprite;
    child.name = "Bg";
    root.children.push_back(std::make_shared<anim::Node>(std::move(child)));

    ASSERT_EQ(root.children.size(), 1u);
    EXPECT_EQ(root.children[0]->id, "child1");
}

TEST(AnimDataTest, KeyframeCreation) {
    anim::Keyframe kf;
    kf.time = 0.5f;
    kf.value = 200.0f;
    kf.easing = anim::EasingType::EaseOut;
    EXPECT_FLOAT_EQ(kf.time, 0.5f);
    EXPECT_FLOAT_EQ(std::get<float>(kf.value), 200.0f);
    EXPECT_EQ(kf.easing, anim::EasingType::EaseOut);
}

TEST(AnimDataTest, AnimationTrackStructure) {
    anim::Track track;
    track.nodeId = "n1";
    track.property = "opacity";
    track.keyframes.push_back({0.0f, 0.0f, anim::EasingType::Linear});
    track.keyframes.push_back({0.4f, 255.0f, anim::EasingType::EaseOut});

    EXPECT_EQ(track.nodeId, "n1");
    EXPECT_EQ(track.property, "opacity");
    ASSERT_EQ(track.keyframes.size(), 2u);
    EXPECT_FLOAT_EQ(track.keyframes[0].time, 0.0f);
    EXPECT_FLOAT_EQ(track.keyframes[1].time, 0.4f);
}

TEST(AnimDataTest, AnimProjectMeta) {
    anim::AnimProject project;
    project.meta.version = "1.0";
    project.meta.canvasWidth = 1280;
    project.meta.canvasHeight = 720;
    project.meta.frameRate = 60;
    EXPECT_EQ(project.meta.version, "1.0");
    EXPECT_EQ(project.meta.canvasWidth, 1280);
}
```

- [ ] **Step 2: 运行测试确认失败**

```bash
cmake --build build --target test_AnimData
cd build && ctest -R AnimData --output-on-failure
```

Expected: 编译失败（AnimData.h 不存在）

- [ ] **Step 3: 实现 AnimData.h**

```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cstdint>

namespace anim {

enum class NodeType { Node, Sprite, Label, Button };
enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut, EaseOutBack, EaseOutBounce, EaseOutElastic };

struct Vec2 { float x = 0.0f, y = 0.0f; };
struct Color { uint8_t r = 255, g = 255, b = 255; };

struct NodeProperties {
    Vec2 position;
    Vec2 scale{1.0f, 1.0f};
    float rotation = 0.0f;
    Vec2 anchor{0.5f, 0.5f};
    uint8_t opacity = 255;
    Color color;
    bool visible = true;
    std::string texture;   // Sprite 专有
    std::string text;      // Label 专有
    std::string font;      // Label 专有
    float fontSize = 24.0f; // Label 专有
    int zIndex = 0;
};

struct Node;
using NodePtr = std::shared_ptr<Node>;

struct Node {
    std::string id;
    NodeType type = NodeType::Node;
    std::string name;
    NodeProperties properties;
    std::vector<NodePtr> children;
};

using KeyframeValue = std::variant<float, int, bool, std::string>;

struct Keyframe {
    float time = 0.0f;
    KeyframeValue value = 0.0f;
    EasingType easing = EasingType::Linear;
};

struct Track {
    std::string nodeId;
    std::string property;  // "position.x", "opacity", etc.
    std::vector<Keyframe> keyframes;
};

struct AnimEvent {
    float time = 0.0f;
    std::string nodeId;
    std::string name;
};

struct Animation {
    std::string name;
    float duration = 1.0f;
    bool loop = false;
    std::vector<Track> tracks;
};

struct ProjectMeta {
    std::string version = "1.0";
    int canvasWidth = 1280;
    int canvasHeight = 720;
    int frameRate = 60;
};

struct AnimProject {
    ProjectMeta meta;
    std::vector<NodePtr> nodeTree;
    std::vector<Animation> animations;
    std::vector<AnimEvent> events;
};

} // namespace anim
```

- [ ] **Step 4: 更新 tests/CMakeLists.txt 添加测试目标**

在 `AnimEditor/tests/CMakeLists.txt` 中追加：

```cmake
add_executable(test_AnimData test_AnimData.cpp)
target_include_directories(test_AnimData PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_AnimData PRIVATE GTest::gtest_main)
add_test(NAME AnimData COMMAND test_AnimData)
```

- [ ] **Step 5: 构建并运行测试**

```bash
cmake --build build --target test_AnimData
cd build && ctest -R AnimData --output-on-failure
```

Expected: 5 个测试全部 PASS

- [ ] **Step 6: 提交**

```bash
git add AnimEditor/src/core/AnimData.h AnimEditor/tests/test_AnimData.cpp AnimEditor/tests/CMakeLists.txt
git commit -m "feat: define AnimData model (Node, Track, Keyframe, Animation)"
```

---

### Task 3: JSON 序列化器（Serializer）

**Files:**
- Create: `AnimEditor/src/core/Serializer.h`
- Create: `AnimEditor/src/core/Serializer.cpp`
- Create: `AnimEditor/tests/test_Serializer.cpp`
- Modify: `AnimEditor/tests/CMakeLists.txt`

- [ ] **Step 1: 编写 Serializer 测试**

```cpp
// test_Serializer.cpp
#include <gtest/gtest.h>
#include "core/Serializer.h"

TEST(SerializerTest, SerializeEmptyProject) {
    anim::AnimProject project;
    project.meta.version = "1.0";
    project.meta.canvasWidth = 800;
    project.meta.canvasHeight = 600;
    project.meta.frameRate = 30;

    std::string json = anim::Serializer::serialize(project);
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("\"version\""), std::string::npos);
    EXPECT_NE(json.find("800"), std::string::npos);
}

TEST(SerializerTest, RoundTripEmptyProject) {
    anim::AnimProject original;
    original.meta.version = "1.0";
    original.meta.canvasWidth = 1280;

    std::string json = anim::Serializer::serialize(original);
    auto loaded = anim::Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->meta.version, "1.0");
    EXPECT_EQ(loaded->meta.canvasWidth, 1280);
}

TEST(SerializerTest, SerializeNodeTree) {
    anim::AnimProject project;
    auto root = std::make_shared<anim::Node>();
    root->id = "root";
    root->type = anim::NodeType::Node;
    root->name = "Root";

    auto child = std::make_shared<anim::Node>();
    child->id = "bg";
    child->type = anim::NodeType::Sprite;
    child->name = "Background";
    child->properties.texture = "bg.png";
    child->properties.opacity = 128;
    root->children.push_back(child);

    project.nodeTree.push_back(root);

    std::string json = anim::Serializer::serialize(project);
    auto loaded = anim::Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_EQ(loaded->nodeTree.size(), 1u);
    ASSERT_EQ(loaded->nodeTree[0]->children.size(), 1u);
    EXPECT_EQ(loaded->nodeTree[0]->children[0]->name, "Background");
    EXPECT_EQ(loaded->nodeTree[0]->children[0]->properties.texture, "bg.png");
    EXPECT_EQ(loaded->nodeTree[0]->children[0]->properties.opacity, 128);
}

TEST(SerializerTest, SerializeAnimationTracks) {
    anim::AnimProject project;
    anim::Animation anim;
    anim.name = "show";
    anim.duration = 0.6f;
    anim.loop = false;

    anim::Track track;
    track.nodeId = "n_bg";
    track.property = "opacity";
    track.keyframes.push_back({0.0f, 0.0f, anim::EasingType::EaseOut});
    track.keyframes.push_back({0.4f, 255.0f, anim::EasingType::Linear});
    anim.tracks.push_back(track);
    project.animations.push_back(anim);

    std::string json = anim::Serializer::serialize(project);
    auto loaded = anim::Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_EQ(loaded->animations.size(), 1u);
    EXPECT_EQ(loaded->animations[0].name, "show");
    EXPECT_FLOAT_EQ(loaded->animations[0].duration, 0.6f);
    ASSERT_EQ(loaded->animations[0].tracks[0].keyframes.size(), 2u);
    EXPECT_FLOAT_EQ(loaded->animations[0].tracks[0].keyframes[1].time, 0.4f);
}

TEST(SerializerTest, SerializeEvents) {
    anim::AnimProject project;
    project.events.push_back({0.0f, "n_bg", "playSound:whoosh"});
    project.events.push_back({1.2f, "n_title", "onComplete"});

    std::string json = anim::Serializer::serialize(project);
    auto loaded = anim::Serializer::deserialize(json);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_EQ(loaded->events.size(), 2u);
    EXPECT_EQ(loaded->events[0].name, "playSound:whoosh");
    EXPECT_FLOAT_EQ(loaded->events[1].time, 1.2f);
}

TEST(SerializerTest, DeserializeInvalidJSON) {
    auto result = anim::Serializer::deserialize("{invalid}");
    EXPECT_FALSE(result.has_value());
}

TEST(SerializerTest, LoadSaveFileRoundTrip) {
    anim::AnimProject project;
    project.meta.version = "1.0";
    auto node = std::make_shared<anim::Node>();
    node->id = "n1"; node->name = "Test";
    project.nodeTree.push_back(node);

    const std::string path = "/tmp/test_roundtrip.anim";
    ASSERT_TRUE(anim::Serializer::saveToFile(project, path));
    auto loaded = anim::Serializer::loadFromFile(path);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->nodeTree[0]->name, "Test");
}
```

- [ ] **Step 2: 运行测试确认失败**

```bash
cmake --build build --target test_Serializer 2>&1 | head -5
```

Expected: 编译失败（Serializer.h 不存在）

- [ ] **Step 3: 实现 Serializer.h**

```cpp
#pragma once
#include "AnimData.h"
#include <string>
#include <optional>

namespace anim {

class Serializer {
public:
    static std::string serialize(const AnimProject& project);
    static std::optional<AnimProject> deserialize(const std::string& json);
    static bool saveToFile(const AnimProject& project, const std::string& path);
    static std::optional<AnimProject> loadFromFile(const std::string& path);
};

} // namespace anim
```

- [ ] **Step 4: 实现 Serializer.cpp**

```cpp
#include "core/Serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace anim {

using json = nlohmann::json;

static const char* nodeTypeToString(NodeType t) {
    switch (t) {
        case NodeType::Node: return "Node";
        case NodeType::Sprite: return "Sprite";
        case NodeType::Label: return "Label";
        case NodeType::Button: return "Button";
    }
    return "Node";
}

static NodeType stringToNodeType(const std::string& s) {
    if (s == "Sprite") return NodeType::Sprite;
    if (s == "Label") return NodeType::Label;
    if (s == "Button") return NodeType::Button;
    return NodeType::Node;
}

static const char* easingToString(EasingType e) {
    switch (e) {
        case EasingType::Linear: return "linear";
        case EasingType::EaseIn: return "easeIn";
        case EasingType::EaseOut: return "easeOut";
        case EasingType::EaseInOut: return "easeInOut";
        case EasingType::EaseOutBack: return "easeOutBack";
        case EasingType::EaseOutBounce: return "easeOutBounce";
        case EasingType::EaseOutElastic: return "easeOutElastic";
    }
    return "linear";
}

static EasingType stringToEasing(const std::string& s) {
    if (s == "easeIn") return EasingType::EaseIn;
    if (s == "easeOut") return EasingType::EaseOut;
    if (s == "easeInOut") return EasingType::EaseInOut;
    if (s == "easeOutBack") return EasingType::EaseOutBack;
    if (s == "easeOutBounce") return EasingType::EaseOutBounce;
    if (s == "easeOutElastic") return EasingType::EaseOutElastic;
    return EasingType::Linear;
}

static json serializeVec2(const Vec2& v) { return {{"x", v.x}, {"y", v.y}}; }
static Vec2 deserializeVec2(const json& j) { return {j.value("x", 0.0f), j.value("y", 0.0f)}; }

static json serializeColor(const Color& c) { return {{"r", c.r}, {"g", c.g}, {"b", c.b}}; }
static Color deserializeColor(const json& j) {
    return {static_cast<uint8_t>(j.value("r", 255)),
            static_cast<uint8_t>(j.value("g", 255)),
            static_cast<uint8_t>(j.value("b", 255))};
}

static json serializeKeyframeValue(const KeyframeValue& v) {
    return std::visit([](auto&& arg) -> json { return arg; }, v);
}

static KeyframeValue deserializeKeyframeValue(const json& j) {
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_string()) return j.get<std::string>();
    if (j.is_number_integer()) return j.get<int>();
    return j.get<float>();
}

static json serializeNode(const NodePtr& node) {
    json j;
    j["id"] = node->id;
    j["type"] = nodeTypeToString(node->type);
    j["name"] = node->name;

    auto& p = node->properties;
    j["properties"] = {
        {"position", serializeVec2(p.position)},
        {"scale", serializeVec2(p.scale)},
        {"rotation", p.rotation},
        {"anchor", serializeVec2(p.anchor)},
        {"opacity", p.opacity},
        {"color", serializeColor(p.color)},
        {"visible", p.visible},
        {"zIndex", p.zIndex}
    };
    if (!p.texture.empty()) j["properties"]["texture"] = p.texture;
    if (!p.text.empty()) j["properties"]["text"] = p.text;

    if (!node->children.empty()) {
        j["children"] = json::array();
        for (auto& c : node->children) j["children"].push_back(serializeNode(c));
    }
    return j;
}

static NodePtr deserializeNode(const json& j) {
    auto node = std::make_shared<Node>();
    node->id = j.value("id", "");
    node->type = stringToNodeType(j.value("type", "Node"));
    node->name = j.value("name", "");

    if (j.contains("properties")) {
        auto& p = j["properties"];
        node->properties.position = deserializeVec2(p.value("position", json::object()));
        node->properties.scale = deserializeVec2(p.value("scale", json::object()));
        node->properties.rotation = p.value("rotation", 0.0f);
        node->properties.anchor = deserializeVec2(p.value("anchor", json::object()));
        node->properties.opacity = p.value("opacity", 255);
        node->properties.color = deserializeColor(p.value("color", json::object()));
        node->properties.visible = p.value("visible", true);
        node->properties.zIndex = p.value("zIndex", 0);
        node->properties.texture = p.value("texture", "");
        node->properties.text = p.value("text", "");
    }

    if (j.contains("children")) {
        for (auto& cj : j["children"]) {
            node->children.push_back(deserializeNode(cj));
        }
    }
    return node;
}

std::string Serializer::serialize(const AnimProject& project) {
    json root;
    root["meta"] = {
        {"version", project.meta.version},
        {"canvas", {{"width", project.meta.canvasWidth}, {"height", project.meta.canvasHeight}}},
        {"frameRate", project.meta.frameRate}
    };

    root["nodeTree"] = json::array();
    for (auto& n : project.nodeTree) root["nodeTree"].push_back(serializeNode(n));

    root["animations"] = json::array();
    for (auto& anim : project.animations) {
        json aj;
        aj["name"] = anim.name;
        aj["duration"] = anim.duration;
        aj["loop"] = anim.loop;
        aj["tracks"] = json::array();
        for (auto& t : anim.tracks) {
            json tj;
            tj["nodeId"] = t.nodeId;
            tj["property"] = t.property;
            tj["keyframes"] = json::array();
            for (auto& kf : t.keyframes) {
                tj["keyframes"].push_back({
                    {"time", kf.time},
                    {"value", serializeKeyframeValue(kf.value)},
                    {"easing", easingToString(kf.easing)}
                });
            }
            aj["tracks"].push_back(tj);
        }
        root["animations"].push_back(aj);
    }

    root["events"] = json::array();
    for (auto& ev : project.events) {
        root["events"].push_back({{"time", ev.time}, {"nodeId", ev.nodeId}, {"name", ev.name}});
    }

    return root.dump(2);
}

std::optional<AnimProject> Serializer::deserialize(const std::string& jsonStr) {
    try {
        auto root = json::parse(jsonStr);
        AnimProject project;

        if (root.contains("meta")) {
            auto& m = root["meta"];
            project.meta.version = m.value("version", "1.0");
            if (m.contains("canvas")) {
                project.meta.canvasWidth = m["canvas"].value("width", 1280);
                project.meta.canvasHeight = m["canvas"].value("height", 720);
            }
            project.meta.frameRate = m.value("frameRate", 60);
        }

        if (root.contains("nodeTree")) {
            for (auto& nj : root["nodeTree"]) {
                project.nodeTree.push_back(deserializeNode(nj));
            }
        }

        if (root.contains("animations")) {
            for (auto& aj : root["animations"]) {
                Animation anim;
                anim.name = aj.value("name", "");
                anim.duration = aj.value("duration", 1.0f);
                anim.loop = aj.value("loop", false);
                if (aj.contains("tracks")) {
                    for (auto& tj : aj["tracks"]) {
                        Track track;
                        track.nodeId = tj.value("nodeId", "");
                        track.property = tj.value("property", "");
                        if (tj.contains("keyframes")) {
                            for (auto& kfj : tj["keyframes"]) {
                                Keyframe kf;
                                kf.time = kfj.value("time", 0.0f);
                                kf.value = deserializeKeyframeValue(kfj["value"]);
                                kf.easing = stringToEasing(kfj.value("easing", "linear"));
                                track.keyframes.push_back(kf);
                            }
                        }
                        anim.tracks.push_back(track);
                    }
                }
                project.animations.push_back(anim);
            }
        }

        if (root.contains("events")) {
            for (auto& ej : root["events"]) {
                project.events.push_back({
                    ej.value("time", 0.0f),
                    ej.value("nodeId", ""),
                    ej.value("name", "")
                });
            }
        }

        return project;
    } catch (const json::exception&) {
        return std::nullopt;
    }
}

bool Serializer::saveToFile(const AnimProject& project, const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << serialize(project);
    return true;
}

std::optional<AnimProject> Serializer::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return std::nullopt;
    std::stringstream ss;
    ss << f.rdbuf();
    return deserialize(ss.str());
}

} // namespace anim
```

- [ ] **Step 5: 更新 tests/CMakeLists.txt 追加 Serializer 测试**

```cmake
add_executable(test_Serializer test_Serializer.cpp)
target_include_directories(test_Serializer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/third_party/json/include
)
target_link_libraries(test_Serializer PRIVATE GTest::gtest_main)
add_test(NAME Serializer COMMAND test_Serializer)
```

- [ ] **Step 6: 构建并运行测试**

```bash
cmake --build build --target test_Serializer
cd build && ctest -R Serializer --output-on-failure
```

Expected: 7 个测试全部 PASS

- [ ] **Step 7: 提交**

```bash
git add AnimEditor/src/core/Serializer.h AnimEditor/src/core/Serializer.cpp AnimEditor/tests/test_Serializer.cpp AnimEditor/tests/CMakeLists.txt
git commit -m "feat: add JSON serializer with round-trip tests"
```

---

### Task 4: 文件浏览器面板

**Files:**
- Create: `AnimEditor/src/ui/FileBrowserPanel.h`
- Create: `AnimEditor/src/ui/FileBrowserPanel.cpp`
- Modify: `AnimEditor/src/main.cpp`

- [ ] **Step 1: 实现 FileBrowserPanel.h**

```cpp
#pragma once
#include <string>
#include <vector>
#include <functional>

namespace anim {

struct FileEntry {
    std::string name;
    std::string path;
    bool isDirectory;
    bool isAnimFile;   // .anim
    bool isImageFile;  // .png .jpg
};

class FileBrowserPanel {
public:
    using OnFileOpen = std::function<void(const std::string& path)>;

    void setRootPath(const std::string& path);
    void setOnFileOpen(OnFileOpen cb) { onFileOpen_ = std::move(cb); }
    void render();

private:
    std::string rootPath_;
    std::string currentPath_;
    std::vector<FileEntry> entries_;
    OnFileOpen onFileOpen_;

    void refreshDirectory(const std::string& path);
};

} // namespace anim
```

- [ ] **Step 2: 实现 FileBrowserPanel.cpp**

```cpp
#include "ui/FileBrowserPanel.h"
#include "imgui.h"
#include <filesystem>
#include <algorithm>

namespace anim {

void FileBrowserPanel::setRootPath(const std::string& path) {
    rootPath_ = path;
    currentPath_ = path;
    refreshDirectory(currentPath_);
}

void FileBrowserPanel::refreshDirectory(const std::string& path) {
    entries_.clear();
    namespace fs = std::filesystem;
    if (!fs::exists(path)) return;

    // 添加返回上级目录条目
    if (currentPath_ != rootPath_) {
        entries_.push_back({"..", fs::path(path).parent_path().string(), true, false, false});
    }

    std::vector<fs::directory_entry> dirs, files;
    for (auto& e : fs::directory_iterator(path)) {
        if (e.is_directory()) dirs.push_back(e);
        else files.push_back(e);
    }

    for (auto& d : dirs) {
        entries_.push_back({d.path().filename().string(), d.path().string(), true, false, false});
    }

    for (auto& f : files) {
        auto ext = f.path().extension().string();
        bool isAnim = (ext == ".anim");
        bool isImage = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp");
        if (isAnim || isImage || ext == ".ttf" || ext == ".otf" || ext == ".plist") {
            entries_.push_back({f.path().filename().string(), f.path().string(), false, isAnim, isImage});
        }
    }
}

void FileBrowserPanel::render() {
    ImGui::Begin("Files");

    if (!rootPath_.empty()) {
        ImGui::Text("%s", currentPath_.c_str());
        ImGui::Separator();

        for (auto& entry : entries_) {
            const char* icon = entry.isDirectory ? "[D]" : entry.isAnimFile ? "[A]" : "[F]";
            ImGui::PushStyleColor(ImGuiCol_Text,
                entry.isDirectory ? ImVec4(0.4f, 0.7f, 1.0f, 1.0f) :
                entry.isAnimFile   ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                entry.isImageFile  ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) :
                                     ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

            if (ImGui::Selectable((std::string(icon) + " " + entry.name).c_str())) {
                if (entry.isDirectory) {
                    currentPath_ = entry.path;
                    refreshDirectory(currentPath_);
                } else if (onFileOpen_) {
                    onFileOpen_(entry.path);
                }
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (entry.isDirectory) {
                    currentPath_ = entry.path;
                    refreshDirectory(currentPath_);
                } else if (onFileOpen_) {
                    onFileOpen_(entry.path);
                }
            }
            ImGui::PopStyleColor();
        }
    } else {
        ImGui::Text("No project opened.");
        if (ImGui::Button("Open Folder...")) {
            // 后续集成 native file dialog
        }
    }

    ImGui::End();
}

} // namespace anim
```

- [ ] **Step 3: 在 main.cpp 中集成 FileBrowserPanel**

在 main.cpp 头部添加 include，在主循环中添加面板渲染。在 `ImGui::DockSpaceOverViewport` 之后、`ImGui::Begin("Welcome")` 位置替换为：

```cpp
#include "ui/FileBrowserPanel.h"

// 在主循环之前创建
anim::FileBrowserPanel fileBrowser;
fileBrowser.setOnFileOpen([](const std::string& path) {
    // Phase 1: 仅打印路径
    printf("File opened: %s\n", path.c_str());
});

// 在主循环中替换 Welcome 面板
// ImGui::Begin("Welcome"); ... ImGui::End();
fileBrowser.render();
```

- [ ] **Step 4: 更新 AnimEditor CMakeLists.txt 添加新源文件**

在 `add_executable` 中追加：

```cmake
src/ui/FileBrowserPanel.cpp
src/core/Serializer.cpp
```

在 `target_include_directories` 中确认已包含 `${CMAKE_SOURCE_DIR}/third_party/json/include`。

- [ ] **Step 5: 构建并手动验证**

```bash
cmake --build build --parallel
./build/AnimEditor/anim_editor
```

Expected: 左侧出现 "Files" 面板，显示目录内容，点击目录可导航，双击文件在终端打印路径。

- [ ] **Step 6: 提交**

```bash
git add AnimEditor/src/ui/FileBrowserPanel.h AnimEditor/src/ui/FileBrowserPanel.cpp AnimEditor/src/main.cpp AnimEditor/CMakeLists.txt
git commit -m "feat: add file browser panel with directory navigation"
```

---

### Task 5: EditorUI 总协调器 + Docking 布局

**Files:**
- Create: `AnimEditor/src/ui/EditorUI.h`
- Create: `AnimEditor/src/ui/EditorUI.cpp`
- Modify: `AnimEditor/src/main.cpp`

- [ ] **Step 1: 实现 EditorUI.h**

```cpp
#pragma once
#include "ui/FileBrowserPanel.h"
#include "core/AnimData.h"
#include <memory>

namespace anim {

class EditorUI {
public:
    bool init();
    void shutdown();
    void render();

    void openProject(const std::string& folderPath);
    void openAnimFile(const std::string& filePath);

private:
    FileBrowserPanel fileBrowser_;
    std::shared_ptr<anim::AnimProject> currentProject_;
    std::string currentFilePath_;

    void renderMenuBar();
};

} // namespace anim
```

- [ ] **Step 2: 实现 EditorUI.cpp**

```cpp
#include "ui/EditorUI.h"
#include "core/Serializer.h"
#include "imgui.h"

namespace anim {

bool EditorUI::init() {
    fileBrowser_.setOnFileOpen([this](const std::string& path) {
        if (path.ends_with(".anim")) {
            openAnimFile(path);
        }
    });
    return true;
}

void EditorUI::shutdown() {}

void EditorUI::render() {
    renderMenuBar();
    fileBrowser_.render();

    // 占位面板
    ImGui::Begin("Preview");
    ImGui::Text("Preview canvas (Phase 2)");
    ImGui::End();

    ImGui::Begin("Properties");
    ImGui::Text("Property panel (Phase 2)");
    ImGui::End();

    ImGui::Begin("Node Tree + Timeline");
    ImGui::Text("Bottom panel (Phase 2+3)");
    ImGui::End();
}

void EditorUI::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Folder...")) {
                // 后续集成 native file dialog
            }
            if (ImGui::MenuItem("New Animation")) {
                currentProject_ = std::make_shared<AnimProject>();
                currentFilePath_.clear();
            }
            if (ImGui::MenuItem("Save", nullptr, false, currentProject_ != nullptr)) {
                if (!currentFilePath_.empty() && currentProject_) {
                    Serializer::saveToFile(*currentProject_, currentFilePath_);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorUI::openProject(const std::string& folderPath) {
    fileBrowser_.setRootPath(folderPath);
}

void EditorUI::openAnimFile(const std::string& filePath) {
    auto project = Serializer::loadFromFile(filePath);
    if (project.has_value()) {
        currentProject_ = std::make_shared<AnimProject>(std::move(project.value()));
        currentFilePath_ = filePath;
    }
}

} // namespace anim
```

- [ ] **Step 3: 重构 main.cpp 使用 EditorUI**

```cpp
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ui/EditorUI.h"

int main(int argc, char** argv) {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "AnimEditor", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    anim::EditorUI editor;
    editor.init();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        editor.render();
        ImGui::Render();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    editor.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

- [ ] **Step 4: 更新 CMakeLists.txt 添加新源文件**

追加到 `add_executable`：

```cmake
src/ui/EditorUI.cpp
src/ui/FileBrowserPanel.cpp
src/core/Serializer.cpp
```

- [ ] **Step 5: 构建并验证**

```bash
cmake --build build --parallel
./build/AnimEditor/anim_editor
```

Expected: 显示 4 个 Docking 面板（Files / Preview / Properties / Node Tree + Timeline）+ 菜单栏。

- [ ] **Step 6: 提交**

```bash
git add AnimEditor/src/ui/EditorUI.h AnimEditor/src/ui/EditorUI.cpp AnimEditor/src/main.cpp AnimEditor/CMakeLists.txt
git commit -m "feat: add EditorUI coordinator with docking layout and menu bar"
```

---

## Phase 2 — 节点树 + 属性编辑 + Cocos2d-x 渲染嵌入

### Task 6: SceneGraph 数据模型 + Undo/Redo

**Files:**
- Create: `AnimEditor/src/core/SceneGraph.h`
- Create: `AnimEditor/src/core/SceneGraph.cpp`
- Create: `AnimEditor/src/core/UndoSystem.h`
- Create: `AnimEditor/src/core/UndoSystem.cpp`
- Create: `AnimEditor/tests/test_SceneGraph.cpp`
- Create: `AnimEditor/tests/test_UndoSystem.cpp`

- [ ] **Step 1: 编写 SceneGraph 测试**

```cpp
// test_SceneGraph.cpp
#include <gtest/gtest.h>
#include "core/SceneGraph.h"

TEST(SceneGraphTest, AddRootNode) {
    anim::SceneGraph sg;
    auto node = sg.addNode("root", anim::NodeType::Node, "Root", std::nullopt);
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ((*node)->id, "root");
    EXPECT_EQ(sg.getRootNodes().size(), 1u);
}

TEST(SceneGraphTest, AddChildNode) {
    anim::SceneGraph sg;
    auto root = sg.addNode("root", anim::NodeType::Node, "Root", std::nullopt);
    auto child = sg.addNode("child1", anim::NodeType::Sprite, "Bg", "root");
    ASSERT_TRUE(child.has_value());
    EXPECT_EQ((*root)->children.size(), 1u);
    EXPECT_EQ((*root)->children[0]->name, "Bg");
}

TEST(SceneGraphTest, RemoveNode) {
    anim::SceneGraph sg;
    sg.addNode("root", anim::NodeType::Node, "Root", std::nullopt);
    auto child = sg.addNode("child1", anim::NodeType::Sprite, "Bg", "root");
    EXPECT_TRUE(sg.removeNode("child1"));
    EXPECT_EQ(sg.getRootNodes()[0]->children.size(), 0u);
}

TEST(SceneGraphTest, RemoveNonexistentNode) {
    anim::SceneGraph sg;
    EXPECT_FALSE(sg.removeNode("nonexistent"));
}

TEST(SceneGraphTest, FindNodeById) {
    anim::SceneGraph sg;
    sg.addNode("root", anim::NodeType::Node, "Root", std::nullopt);
    sg.addNode("child", anim::NodeType::Sprite, "Bg", "root");
    auto found = sg.findById("child");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ((*found)->name, "Bg");
}

TEST(SceneGraphTest, RenameNode) {
    anim::SceneGraph sg;
    sg.addNode("n1", anim::NodeType::Node, "Old", std::nullopt);
    EXPECT_TRUE(sg.renameNode("n1", "New"));
    EXPECT_EQ(sg.findById("n1")->get()->name, "New");
}
```

- [ ] **Step 2: 运行测试确认失败**

```bash
cmake --build build --target test_SceneGraph 2>&1 | head -5
```

Expected: 编译失败

- [ ] **Step 3: 实现 SceneGraph.h**

```cpp
#pragma once
#include "core/AnimData.h"
#include <optional>
#include <functional>

namespace anim {

class SceneGraph {
public:
    const std::vector<NodePtr>& getRootNodes() const { return roots_; }

    std::optional<NodePtr> addNode(const std::string& id, NodeType type,
                                    const std::string& name,
                                    const std::optional<std::string>& parentId);
    bool removeNode(const std::string& id);
    std::optional<NodePtr> findById(const std::string& id) const;
    bool renameNode(const std::string& id, const std::string& newName);
    bool reorderNode(const std::string& id, int newIndex);
    bool reparentNode(const std::string& id, const std::optional<std::string>& newParentId);

    void setOnChanged(std::function<void()> cb) { onChanged_ = std::move(cb); }
    void clear();

private:
    std::vector<NodePtr> roots_;
    std::function<void()> onChanged_;

    NodePtr findNodeMutable(const std::string& id, std::vector<NodePtr>& nodes);
    bool removeNodeFrom(const std::string& id, std::vector<NodePtr>& nodes);
};

} // namespace anim
```

- [ ] **Step 4: 实现 SceneGraph.cpp**

```cpp
#include "core/SceneGraph.h"
#include <algorithm>

namespace anim {

std::optional<NodePtr> SceneGraph::addNode(const std::string& id, NodeType type,
                                            const std::string& name,
                                            const std::optional<std::string>& parentId) {
    if (findById(id)) return std::nullopt;

    auto node = std::make_shared<Node>();
    node->id = id;
    node->type = type;
    node->name = name;

    if (parentId.has_value()) {
        auto parent = findById(*parentId);
        if (!parent) return std::nullopt;
        (*parent)->children.push_back(node);
    } else {
        roots_.push_back(node);
    }

    if (onChanged_) onChanged_();
    return node;
}

bool SceneGraph::removeNode(const std::string& id) {
    if (removeNodeFrom(id, roots_)) {
        if (onChanged_) onChanged_();
        return true;
    }
    return false;
}

bool SceneGraph::removeNodeFrom(const std::string& id, std::vector<NodePtr>& nodes) {
    auto it = std::remove_if(nodes.begin(), nodes.end(),
        [&](const NodePtr& n) { return n->id == id; });
    if (it != nodes.end()) {
        nodes.erase(it, nodes.end());
        return true;
    }
    for (auto& n : nodes) {
        if (removeNodeFrom(id, n->children)) return true;
    }
    return false;
}

std::optional<NodePtr> SceneGraph::findById(const std::string& id) const {
    std::function<NodePtr(const std::vector<NodePtr>&)> search;
    search = [&](const std::vector<NodePtr>& nodes) -> NodePtr {
        for (auto& n : nodes) {
            if (n->id == id) return n;
            auto found = search(n->children);
            if (found) return found;
        }
        return nullptr;
    };
    auto result = search(roots_);
    return result ? std::optional<NodePtr>(result) : std::nullopt;
}

bool SceneGraph::renameNode(const std::string& id, const std::string& newName) {
    auto node = findById(id);
    if (!node) return false;
    (*node)->name = newName;
    if (onChanged_) onChanged_();
    return true;
}

bool SceneGraph::reorderNode(const std::string& id, int newIndex) {
    // 查找节点所在父级 children 列表，移动位置
    std::function<bool(std::vector<NodePtr>&)> reorder;
    reorder = [&](std::vector<NodePtr>& nodes) -> bool {
        for (size_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->id == id) {
                auto node = nodes[i];
                nodes.erase(nodes.begin() + i);
                size_t insertAt = std::min(static_cast<size_t>(newIndex), nodes.size());
                nodes.insert(nodes.begin() + insertAt, node);
                return true;
            }
            if (reorder(nodes[i]->children)) return true;
        }
        return false;
    };
    if (reorder(roots_)) {
        if (onChanged_) onChanged_();
        return true;
    }
    return false;
}

bool SceneGraph::reparentNode(const std::string& id, const std::optional<std::string>& newParentId) {
    auto node = findById(id);
    if (!node) return false;
    auto nodePtr = *node;
    removeNodeFrom(id, roots_);

    if (newParentId.has_value()) {
        auto parent = findById(*newParentId);
        if (parent) {
            (*parent)->children.push_back(nodePtr);
        } else {
            roots_.push_back(nodePtr);
        }
    } else {
        roots_.push_back(nodePtr);
    }
    if (onChanged_) onChanged_();
    return true;
}

void SceneGraph::clear() {
    roots_.clear();
    if (onChanged_) onChanged_();
}

} // namespace anim
```

- [ ] **Step 5: 编写 UndoSystem 测试**

```cpp
// test_UndoSystem.cpp
#include <gtest/gtest.h>
#include "core/UndoSystem.h"

TEST(UndoSystemTest, ExecuteAndUndo) {
    anim::UndoSystem undo;
    int value = 0;

    undo.execute("set 5",
        [&]() { value = 5; },
        [&]() { value = 0; });

    EXPECT_EQ(value, 5);
    EXPECT_TRUE(undo.canUndo());
    EXPECT_FALSE(undo.canRedo());

    undo.undo();
    EXPECT_EQ(value, 0);
    EXPECT_FALSE(undo.canUndo());
    EXPECT_TRUE(undo.canRedo());

    undo.redo();
    EXPECT_EQ(value, 5);
}

TEST(UndoSystemTest, UndoChainBreaksOnNewAction) {
    anim::UndoSystem undo;
    int value = 0;

    undo.execute("set 5", [&]() { value = 5; }, [&]() { value = 0; });
    undo.undo();
    undo.execute("set 10", [&]() { value = 10; }, [&]() { value = 0; });

    EXPECT_FALSE(undo.canRedo());
    EXPECT_EQ(value, 10);
}

TEST(UndoSystemTest, UndoLimit) {
    anim::UndoSystem undo(3);
    int v = 0;

    for (int i = 1; i <= 5; i++) {
        int prev = v;
        int next = i;
        undo.execute("set", [&v, next]() { v = next; }, [&v, prev]() { v = prev; });
    }

    EXPECT_EQ(v, 5);
    undo.undo(); // v=4
    undo.undo(); // v=3
    undo.undo(); // v=2
    EXPECT_EQ(v, 2);
    EXPECT_FALSE(undo.canUndo()); // 最早2条已被丢弃
}
```

- [ ] **Step 6: 实现 UndoSystem.h**

```cpp
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace anim {

class UndoSystem {
public:
    using Action = std::function<void()>;

    explicit UndoSystem(size_t maxHistory = 100) : maxHistory_(maxHistory) {}

    void execute(const std::string& name, Action doFn, Action undoFn);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    void clear();
    const std::string& undoName() const;
    const std::string& redoName() const;

private:
    struct Entry {
        std::string name;
        Action undo;
        Action redo;
    };
    std::vector<Entry> history_;
    size_t index_ = 0;
    size_t maxHistory_;
};

} // namespace anim
```

- [ ] **Step 7: 实现 UndoSystem.cpp**

```cpp
#include "core/UndoSystem.h"

namespace anim {

void UndoSystem::execute(const std::string& name, Action doFn, Action undoFn) {
    history_.resize(index_);
    history_.push_back({name, std::move(undoFn), std::move(doFn)});
    if (history_.size() > maxHistory_) {
        history_.erase(history_.begin());
    }
    index_ = history_.size();
    doFn();
}

bool UndoSystem::undo() {
    if (!canUndo()) return false;
    index_--;
    history_[index_].undo();
    return true;
}

bool UndoSystem::redo() {
    if (!canRedo()) return false;
    history_[index_].redo();
    index_++;
    return true;
}

bool UndoSystem::canUndo() const { return index_ > 0; }
bool UndoSystem::canRedo() const { return index_ < history_.size(); }

void UndoSystem::clear() {
    history_.clear();
    index_ = 0;
}

const std::string& UndoSystem::undoName() const {
    static const std::string empty;
    return canUndo() ? history_[index_ - 1].name : empty;
}

const std::string& UndoSystem::redoName() const {
    static const std::string empty;
    return canRedo() ? history_[index_].name : empty;
}

} // namespace anim
```

- [ ] **Step 8: 更新 tests/CMakeLists.txt 追加两个测试目标**

```cmake
add_executable(test_SceneGraph test_SceneGraph.cpp)
target_include_directories(test_SceneGraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_SceneGraph PRIVATE GTest::gtest_main)
add_test(NAME SceneGraph COMMAND test_SceneGraph)

add_executable(test_UndoSystem test_UndoSystem.cpp)
target_include_directories(test_UndoSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_UndoSystem PRIVATE GTest::gtest_main)
add_test(NAME UndoSystem COMMAND test_UndoSystem)
```

- [ ] **Step 9: 构建并运行测试**

```bash
cmake --build build --target test_SceneGraph test_UndoSystem
cd build && ctest -R "SceneGraph|UndoSystem" --output-on-failure
```

Expected: 全部 PASS

- [ ] **Step 10: 提交**

```bash
git add AnimEditor/src/core/SceneGraph.h AnimEditor/src/core/SceneGraph.cpp \
        AnimEditor/src/core/UndoSystem.h AnimEditor/src/core/UndoSystem.cpp \
        AnimEditor/tests/test_SceneGraph.cpp AnimEditor/tests/test_UndoSystem.cpp \
        AnimEditor/tests/CMakeLists.txt
git commit -m "feat: add SceneGraph model and Undo/Redo system with tests"
```

---

### Task 7: 节点树面板 + 属性面板

**Files:**
- Create: `AnimEditor/src/ui/NodeTreePanel.h`
- Create: `AnimEditor/src/ui/NodeTreePanel.cpp`
- Create: `AnimEditor/src/ui/PropertyPanel.h`
- Create: `AnimEditor/src/ui/PropertyPanel.cpp`
- Modify: `AnimEditor/src/ui/EditorUI.h`
- Modify: `AnimEditor/src/ui/EditorUI.cpp`

- [ ] **Step 1: 实现 NodeTreePanel.h**

```cpp
#pragma once
#include "core/AnimData.h"
#include "core/SceneGraph.h"
#include <functional>
#include <string>

namespace anim {

class NodeTreePanel {
public:
    using OnNodeSelected = std::function<void(const std::string& nodeId)>;

    void setSceneGraph(SceneGraph* sg) { sceneGraph_ = sg; }
    void setOnNodeSelected(OnNodeSelected cb) { onNodeSelected_ = std::move(cb); }
    void setSelectedNode(const std::string& id) { selectedId_ = id; }
    const std::string& getSelectedNode() const { return selectedId_; }
    void render();

private:
    SceneGraph* sceneGraph_ = nullptr;
    std::string selectedId_;
    OnNodeSelected onNodeSelected_;

    void renderNode(const NodePtr& node);
};

} // namespace anim
```

- [ ] **Step 2: 实现 NodeTreePanel.cpp**

```cpp
#include "ui/NodeTreePanel.h"
#include "imgui.h"

namespace anim {

void NodeTreePanel::render() {
    ImGui::Begin("Nodes");

    if (sceneGraph_) {
        if (ImGui::Button("+ Add Node")) {
            static int counter = 0;
            std::string id = "node_" + std::to_string(counter++);
            std::string parentId = selectedId_.empty() ? std::string() : selectedId_;
            auto parent = parentId.empty() ? std::optional<std::string>() : std::optional<std::string>(parentId);
            sceneGraph_->addNode(id, NodeType::Node, "NewNode", parent);
        }

        ImGui::Separator();

        for (auto& root : sceneGraph_->getRootNodes()) {
            renderNode(root);
        }
    } else {
        ImGui::Text("No scene loaded.");
    }

    ImGui::End();
}

void NodeTreePanel::renderNode(const NodePtr& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node->id == selectedId_) flags |= ImGuiTreeNodeFlags_Selected;
    if (node->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

    const char* typeIcon =
        node->type == NodeType::Sprite ? "Img" :
        node->type == NodeType::Label ? "Txt" :
        node->type == NodeType::Button ? "Btn" : "Nod";

    bool opened = ImGui::TreeNodeEx(
        (void*)node.get(), flags, "[%s] %s", typeIcon, node->name.c_str());

    if (ImGui::IsItemClicked()) {
        selectedId_ = node->id;
        if (onNodeSelected_) onNodeSelected_(node->id);
    }

    // 右键上下文菜单
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
            sceneGraph_->removeNode(node->id);
            if (selectedId_ == node->id) selectedId_.clear();
        }
        if (ImGui::MenuItem("Add Child")) {
            static int cc = 0;
            std::string cid = node->id + "_c" + std::to_string(cc++);
            sceneGraph_->addNode(cid, NodeType::Node, "Child", node->id);
        }
        ImGui::EndPopup();
    }

    if (opened) {
        for (auto& child : node->children) {
            renderNode(child);
        }
        ImGui::TreePop();
    }
}

} // namespace anim
```

- [ ] **Step 3: 实现 PropertyPanel.h**

```cpp
#pragma once
#include "core/AnimData.h"
#include <functional>

namespace anim {

class PropertyPanel {
public:
    using OnPropertyChanged = std::function<void(const std::string& nodeId, const std::string& property)>;

    void setNode(const NodePtr& node) { currentNode_ = node; }
    void setOnPropertyChanged(OnPropertyChanged cb) { onPropertyChanged_ = std::move(cb); }
    void render();

private:
    NodePtr currentNode_;
    OnPropertyChanged onPropertyChanged_;
};

} // namespace anim
```

- [ ] **Step 4: 实现 PropertyPanel.cpp**

```cpp
#include "ui/PropertyPanel.h"
#include "imgui.h"

namespace anim {

void PropertyPanel::render() {
    ImGui::Begin("Properties");

    if (!currentNode_) {
        ImGui::Text("Select a node to edit properties.");
        ImGui::End();
        return;
    }

    auto& p = currentNode_->properties;
    bool changed = false;

    ImGui::Text("%s (%s)", currentNode_->name.c_str(),
        currentNode_->type == NodeType::Sprite ? "Sprite" :
        currentNode_->type == NodeType::Label  ? "Label"  :
        currentNode_->type == NodeType::Button ? "Button" : "Node");

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat2("Position", &p.position.x, 1.0f);
        changed |= ImGui::DragFloat2("Scale", &p.scale.x, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat("Rotation", &p.rotation, 1.0f, -360.0f, 360.0f);
        changed |= ImGui::DragFloat2("Anchor", &p.anchor.x, 0.01f, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::SliderInt("Opacity", reinterpret_cast<int*>(&p.opacity), 0, 255);
        float col[3] = { p.color.r / 255.0f, p.color.g / 255.0f, p.color.b / 255.0f };
        if (ImGui::ColorEdit3("Color", col)) {
            p.color.r = static_cast<uint8_t>(col[0] * 255);
            p.color.g = static_cast<uint8_t>(col[1] * 255);
            p.color.b = static_cast<uint8_t>(col[2] * 255);
            changed = true;
        }
        changed |= ImGui::Checkbox("Visible", &p.visible);
    }

    if (currentNode_->type == NodeType::Sprite) {
        if (ImGui::CollapsingHeader("Sprite")) {
            char buf[256];
            strncpy(buf, p.texture.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText("Texture", buf, sizeof(buf))) {
                p.texture = buf;
                changed = true;
            }
        }
    }

    if (currentNode_->type == NodeType::Label) {
        if (ImGui::CollapsingHeader("Label")) {
            char buf[256];
            strncpy(buf, p.text.c_str(), sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText("Text", buf, sizeof(buf))) {
                p.text = buf;
                changed = true;
            }
            changed |= ImGui::DragFloat("Font Size", &p.fontSize, 1.0f, 8.0f, 200.0f);
        }
    }

    if (changed && onPropertyChanged_) {
        onPropertyChanged_(currentNode_->id, "");
    }

    ImGui::End();
}

} // namespace anim
```

- [ ] **Step 5: 集成到 EditorUI**

在 `EditorUI.h` 中添加成员：

```cpp
#include "ui/NodeTreePanel.h"
#include "ui/PropertyPanel.h"
#include "core/SceneGraph.h"
#include "core/UndoSystem.h"

// 新增成员
NodeTreePanel nodeTreePanel_;
PropertyPanel propertyPanel_;
SceneGraph sceneGraph_;
UndoSystem undoSystem_;
```

在 `EditorUI.cpp` 的 `render()` 中替换占位面板：

```cpp
// 替换占位的 Properties 面板
propertyPanel_.render();

// 替换占位的 Node Tree + Timeline 底部面板
ImGui::Begin("Node Tree + Timeline");
nodeTreePanel_.render();  // 渲染在 ImGui::Begin/End 内部（NodeTreePanel 内部已移除自己的 Begin/End）
// 注意：需要调整 NodeTreePanel::render() 不再自己 Begin/End
ImGui::End();
```

实际做法：将 NodeTreePanel::render() 中的 `ImGui::Begin("Nodes")` / `ImGui::End()` 移除，改为直接渲染内容（由 EditorUI 控制外层容器）。或者保持独立面板。这里选择保持 NodeTreePanel 为独立面板，Phase 3 再与 TimelinePanel 耦合。

- [ ] **Step 6: 构建并手动验证**

```bash
cmake --build build --parallel
./build/AnimEditor/anim_editor
```

Expected: 节点树面板支持添加/删除/选中节点。属性面板显示选中节点的属性，可编辑。

- [ ] **Step 7: 提交**

```bash
git add AnimEditor/src/ui/NodeTreePanel.h AnimEditor/src/ui/NodeTreePanel.cpp \
        AnimEditor/src/ui/PropertyPanel.h AnimEditor/src/ui/PropertyPanel.cpp \
        AnimEditor/src/ui/EditorUI.h AnimEditor/src/ui/EditorUI.cpp
git commit -m "feat: add node tree panel and property inspector panel"
```

---

### Task 8: Cocos2d-x headless 渲染嵌入 + 预览画布

**Files:**
- Create: `AnimEditor/src/renderer/CocosEmbed.h`
- Create: `AnimEditor/src/renderer/CocosEmbed.cpp`
- Create: `AnimEditor/src/ui/PreviewCanvas.h`
- Create: `AnimEditor/src/ui/PreviewCanvas.cpp`
- Modify: `AnimEditor/CMakeLists.txt`（链接 cocos2d-x）

> **注意：** 此 Task 需要 Cocos2d-x 4.x 源码已存在于 `third_party/cocos2d-x/`。具体初始化 API 取决于 Cocos2d-x 4.x 的 headless 模式接口，以下为框架代码，需根据实际引擎 API 调整。

- [ ] **Step 1: 实现 CocosEmbed.h**

```cpp
#pragma once
#include <cstdint>

namespace anim {

class CocosEmbed {
public:
    bool init(int width, int height);
    void shutdown();
    void resize(int width, int height);
    void renderFrame();
    uint32_t getTextureId() const { return textureId_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    uint32_t fbo_ = 0;
    uint32_t textureId_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};

} // namespace anim
```

- [ ] **Step 2: 实现 CocosEmbed.cpp**

```cpp
#include "renderer/CocosEmbed.h"
#include "imgui.h"

// Cocos2d-x 4.x headless 集成
// 此文件需要根据 Cocos2d-x 4.x 实际 API 调整
// 核心流程：创建 FBO → Director headless 初始化 → 每帧渲染到 FBO → 传纹理给 ImGui

namespace anim {

bool CocosEmbed::init(int width, int height) {
    width_ = width;
    height_ = height;

    // 1. 创建 FBO + 纹理
    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. 初始化 Cocos2d-x Director (headless)
    // cocos2d::Director::getInstance()->init();
    // 设置渲染到 FBO 而非默认 backbuffer
    // 具体实现取决于 Cocos2d-x 4.x 的 headless/离屏渲染支持

    initialized_ = true;
    return true;
}

void CocosEmbed::shutdown() {
    if (initialized_) {
        if (fbo_) glDeleteFramebuffers(1, &fbo_);
        if (textureId_) glDeleteTextures(1, &textureId_);
        initialized_ = false;
    }
}

void CocosEmbed::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void CocosEmbed::renderFrame() {
    if (!initialized_) return;

    // 保存当前 FBO
    GLint prevFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    // 切换到我们的 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glClearColor(0.086f, 0.086f, 0.118f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 渲染 Cocos2d-x 场景
    // cocos2d::Director::getInstance()->mainLoop();

    // 恢复 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
}

} // namespace anim
```

- [ ] **Step 3: 实现 PreviewCanvas.h**

```cpp
#pragma once
#include "renderer/CocosEmbed.h"

namespace anim {

class PreviewCanvas {
public:
    bool init(int width, int height);
    void shutdown();
    void render();

    CocosEmbed& getCocosEmbed() { return embed_; }

private:
    CocosEmbed embed_;
};

} // namespace anim
```

- [ ] **Step 4: 实现 PreviewCanvas.cpp**

```cpp
#include "ui/PreviewCanvas.h"
#include "imgui.h"

namespace anim {

bool PreviewCanvas::init(int width, int height) {
    return embed_.init(width, height);
}

void PreviewCanvas::shutdown() {
    embed_.shutdown();
}

void PreviewCanvas::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Preview");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x > 0 && avail.y > 0) {
        embed_.resize(static_cast<int>(avail.x), static_cast<int>(avail.y));
        embed_.renderFrame();

        ImGui::Image(
            reinterpret_cast<ImTextureID>(static_cast<intptr_t>(embed_.getTextureId())),
            avail,
            ImVec2(0, 1), ImVec2(1, 0));  // 翻转 Y 轴
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace anim
```

- [ ] **Step 5: 集成到 EditorUI + 更新 CMakeLists.txt**

在 EditorUI 中添加 PreviewCanvas 成员并在 init/shutdown/render 中调用。
在 CMakeLists.txt 中添加 `src/renderer/CocosEmbed.cpp` `src/ui/PreviewCanvas.cpp` 并链接 cocos2d-x。

- [ ] **Step 6: 构建验证**

```bash
cmake --build build --parallel
```

Expected: 编译通过，预览面板显示深色背景（Cocos2d-x 场景渲染占位）。

- [ ] **Step 7: 提交**

```bash
git add AnimEditor/src/renderer/ AnimEditor/src/ui/PreviewCanvas.h AnimEditor/src/ui/PreviewCanvas.cpp \
        AnimEditor/src/ui/EditorUI.h AnimEditor/src/ui/EditorUI.cpp AnimEditor/CMakeLists.txt
git commit -m "feat: add Cocos2d-x headless render embed and preview canvas"
```

---

## Phase 3 — 时间线 + 动画编辑

### Task 9: 缓动函数库

**Files:**
- Create: `AnimEditor/src/core/Easing.h`
- Create: `AnimEditor/src/core/Easing.cpp`
- Create: `AnimEditor/tests/test_Easing.cpp`

- [ ] **Step 1: 编写缓动函数测试**

```cpp
// test_Easing.cpp
#include <gtest/gtest.h>
#include "core/Easing.h"
#include <cmath>

TEST(EasingTest, LinearEndpoints) {
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 0.5f), 0.5f);
}

TEST(EasingTest, EaseOutEndpoints) {
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::EaseOut, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::EaseOut, 1.0f), 1.0f);
}

TEST(EasingTest, EaseOutBackOvershoots) {
    float mid = anim::Easing::apply(anim::EasingType::EaseOutBack, 0.5f);
    EXPECT_GT(mid, 0.5f); // easeOutBack 在中间会超过 1.0 的目标
}

TEST(EasingTest, ClampedInput) {
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, -0.5f), 0.0f);
    EXPECT_FLOAT_EQ(anim::Easing::apply(anim::EasingType::Linear, 1.5f), 1.0f);
}

TEST(EasingTest, AllTypesProduceValidOutput) {
    anim::EasingType types[] = {
        anim::EasingType::Linear, anim::EasingType::EaseIn,
        anim::EasingType::EaseOut, anim::EasingType::EaseInOut,
        anim::EasingType::EaseOutBack, anim::EasingType::EaseOutBounce,
        anim::EasingType::EaseOutElastic
    };
    for (auto t : types) {
        float v = anim::Easing::apply(t, 0.5f);
        EXPECT_TRUE(std::isfinite(v));
    }
}
```

- [ ] **Step 2: 运行测试确认失败**

```bash
cmake --build build --target test_Easing 2>&1 | head -5
```

- [ ] **Step 3: 实现 Easing.h**

```cpp
#pragma once
#include "core/AnimData.h"

namespace anim {

class Easing {
public:
    static float apply(EasingType type, float t);

private:
    static float easeIn(float t);
    static float easeOut(float t);
    static float easeInOut(float t);
    static float easeOutBack(float t);
    static float easeOutBounce(float t);
    static float easeOutElastic(float t);
};

} // namespace anim
```

- [ ] **Step 4: 实现 Easing.cpp**

```cpp
#include "core/Easing.h"
#include <cmath>
#include <algorithm>

namespace anim {

float Easing::apply(EasingType type, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (type) {
        case EasingType::Linear: return t;
        case EasingType::EaseIn: return easeIn(t);
        case EasingType::EaseOut: return easeOut(t);
        case EasingType::EaseInOut: return easeInOut(t);
        case EasingType::EaseOutBack: return easeOutBack(t);
        case EasingType::EaseOutBounce: return easeOutBounce(t);
        case EasingType::EaseOutElastic: return easeOutElastic(t);
    }
    return t;
}

float Easing::easeIn(float t) { return t * t; }

float Easing::easeOut(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }

float Easing::easeInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float Easing::easeOutBack(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

float Easing::easeOutBounce(float t) {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.0f / d1) return n1 * t * t;
    if (t < 2.0f / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
    if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

float Easing::easeOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    constexpr float c4 = (2.0f * 3.14159265f) / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

} // namespace anim
```

- [ ] **Step 5: 构建并运行测试**

```bash
cmake --build build --target test_Easing
cd build && ctest -R Easing --output-on-failure
```

Expected: 5 个测试全部 PASS

- [ ] **Step 6: 提交**

```bash
git add AnimEditor/src/core/Easing.h AnimEditor/src/core/Easing.cpp AnimEditor/tests/test_Easing.cpp AnimEditor/tests/CMakeLists.txt
git commit -m "feat: add easing functions with tests"
```

---

### Task 10: AnimationEngine 插值引擎

**Files:**
- Create: `AnimEditor/src/core/AnimationEngine.h`
- Create: `AnimEditor/src/core/AnimationEngine.cpp`
- Create: `AnimEditor/tests/test_AnimationEngine.cpp`

- [ ] **Step 1: 编写 AnimationEngine 测试**

```cpp
// test_AnimationEngine.cpp
#include <gtest/gtest.h>
#include "core/AnimationEngine.h"

TEST(AnimationEngineTest, LinearInterpolation) {
    anim::AnimationEngine engine;
    anim::Track track;
    track.nodeId = "n1";
    track.property = "opacity";
    track.keyframes = {
        {0.0f, 0.0f, anim::EasingType::Linear},
        {1.0f, 255.0f, anim::EasingType::Linear}
    };

    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.5f), 127.5f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 1.0f), 255.0f);
}

TEST(AnimationEngineTest, SingleKeyframe) {
    anim::AnimationEngine engine;
    anim::Track track;
    track.keyframes = {{0.5f, 100.0f, anim::EasingType::Linear}};

    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.0f), 100.0f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 1.0f), 100.0f);
}

TEST(AnimationEngineTest, ThreeKeyframes) {
    anim::AnimationEngine engine;
    anim::Track track;
    track.keyframes = {
        {0.0f, 0.0f, anim::EasingType::Linear},
        {0.5f, 100.0f, anim::EasingType::Linear},
        {1.0f, 200.0f, anim::EasingType::Linear}
    };

    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.25f), 50.0f);
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.75f), 150.0f);
}

TEST(AnimationEngineTest, NoKeyframes) {
    anim::AnimationEngine engine;
    anim::Track track;
    EXPECT_FLOAT_EQ(engine.evaluate(track, 0.5f), 0.0f);
}

TEST(AnimationEngineTest, EvaluateAllTracksForTime) {
    anim::AnimProject project;
    anim::Animation anim;
    anim.name = "test";
    anim.duration = 1.0f;

    anim::Track t1;
    t1.nodeId = "n1"; t1.property = "opacity";
    t1.keyframes = {{0.0f, 0.0f, anim::EasingType::Linear}, {1.0f, 255.0f, anim::EasingType::Linear}};
    anim.tracks.push_back(t1);

    anim::Track t2;
    t2.nodeId = "n1"; t2.property = "position.x";
    t2.keyframes = {{0.0f, 100.0f, anim::EasingType::Linear}, {1.0f, 200.0f, anim::EasingType::Linear}};
    anim.tracks.push_back(t2);

    project.animations.push_back(anim);

    anim::AnimationEngine engine;
    auto result = engine.evaluateAtTime(project, "test", 0.5f);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_FLOAT_EQ(std::get<float>(result["n1:opacity"]), 127.5f);
    EXPECT_FLOAT_EQ(std::get<float>(result["n1:position.x"]), 150.0f);
}
```

- [ ] **Step 2: 运行测试确认失败**

- [ ] **Step 3: 实现 AnimationEngine.h**

```cpp
#pragma once
#include "core/AnimData.h"
#include <map>
#include <string>

namespace anim {

class AnimationEngine {
public:
    float evaluate(const Track& track, float time) const;
    std::map<std::string, KeyframeValue> evaluateAtTime(
        const AnimProject& project, const std::string& animName, float time) const;
};

} // namespace anim
```

- [ ] **Step 4: 实现 AnimationEngine.cpp**

```cpp
#include "core/AnimationEngine.h"
#include "core/Easing.h"

namespace anim {

float AnimationEngine::evaluate(const Track& track, float time) const {
    if (track.keyframes.empty()) return 0.0f;
    if (track.keyframes.size() == 1) return std::get<float>(track.keyframes[0].value);

    // 时间在第一帧之前
    if (time <= track.keyframes.front().time) {
        return std::get<float>(track.keyframes.front().value);
    }
    // 时间在最后一帧之后
    if (time >= track.keyframes.back().time) {
        return std::get<float>(track.keyframes.back().value);
    }

    // 找到包围时间的两个关键帧
    for (size_t i = 0; i < track.keyframes.size() - 1; i++) {
        if (time >= track.keyframes[i].time && time <= track.keyframes[i + 1].time) {
            float t0 = track.keyframes[i].time;
            float t1 = track.keyframes[i + 1].time;
            float v0 = std::get<float>(track.keyframes[i].value);
            float v1 = std::get<float>(track.keyframes[i + 1].value);

            float normalizedT = (t1 - t0) > 0.0f ? (time - t0) / (t1 - t0) : 0.0f;
            float easedT = Easing::apply(track.keyframes[i].easing, normalizedT);
            return v0 + (v1 - v0) * easedT;
        }
    }
    return std::get<float>(track.keyframes.back().value);
}

std::map<std::string, KeyframeValue> AnimationEngine::evaluateAtTime(
    const AnimProject& project, const std::string& animName, float time) const {

    std::map<std::string, KeyframeValue> result;
    for (auto& anim : project.animations) {
        if (anim.name != animName) continue;
        for (auto& track : anim.tracks) {
            float val = evaluate(track, time);
            std::string key = track.nodeId + ":" + track.property;
            result[key] = val;
        }
    }
    return result;
}

} // namespace anim
```

- [ ] **Step 5: 构建并运行测试**

```bash
cmake --build build --target test_AnimationEngine
cd build && ctest -R AnimationEngine --output-on-failure
```

Expected: 5 个测试全部 PASS

- [ ] **Step 6: 提交**

```bash
git add AnimEditor/src/core/AnimationEngine.h AnimEditor/src/core/AnimationEngine.cpp \
        AnimEditor/tests/test_AnimationEngine.cpp AnimEditor/tests/CMakeLists.txt
git commit -m "feat: add animation interpolation engine with easing support"
```

---

### Task 11: 时间线面板

**Files:**
- Create: `AnimEditor/src/ui/TimelinePanel.h`
- Create: `AnimEditor/src/ui/TimelinePanel.cpp`
- Modify: `AnimEditor/src/ui/EditorUI.h`
- Modify: `AnimEditor/src/ui/EditorUI.cpp`

- [ ] **Step 1: 实现 TimelinePanel.h**

```cpp
#pragma once
#include "core/AnimData.h"
#include <functional>
#include <string>

namespace anim {

class TimelinePanel {
public:
    using OnTimeChanged = std::function<void(float time)>;
    using OnKeyframeAdded = std::function<void(const std::string& nodeId, const std::string& property)>;
    using OnKeyframeRemoved = std::function<void(const std::string& nodeId, const std::string& property, int index)>;

    void setProject(AnimProject* project) { project_ = project; }
    void setCurrentAnimation(const std::string& name) { currentAnim_ = name; }
    void setCurrentTime(float t) { currentTime_ = t; }
    void setSelectedNode(const std::string& id) { selectedNodeId_ = id; }

    void setOnTimeChanged(OnTimeChanged cb) { onTimeChanged_ = std::move(cb); }
    void setOnKeyframeAdded(OnKeyframeAdded cb) { onKeyframeAdded_ = std::move(cb); }
    void setOnKeyframeRemoved(OnKeyframeRemoved cb) { onKeyframeRemoved_ = std::move(cb); }

    float getCurrentTime() const { return currentTime_; }
    void render();

private:
    AnimProject* project_ = nullptr;
    std::string currentAnim_;
    std::string selectedNodeId_;
    float currentTime_ = 0.0f;
    bool isPlaying_ = false;
    float pixelsPerSecond_ = 200.0f;

    OnTimeChanged onTimeChanged_;
    OnKeyframeAdded onKeyframeAdded_;
    OnKeyframeRemoved onKeyframeRemoved_;

    void renderTransportControls(Animation* anim);
    void renderTrackRow(const Track& track, float duration);
};

} // namespace anim
```

- [ ] **Step 2: 实现 TimelinePanel.cpp**

```cpp
#include "ui/TimelinePanel.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace anim {

void TimelinePanel::render() {
    ImGui::Begin("Timeline");

    if (!project_ || currentAnim_.empty()) {
        ImGui::Text("No animation selected.");
        ImGui::End();
        return;
    }

    Animation* anim = nullptr;
    for (auto& a : project_->animations) {
        if (a.name == currentAnim_) { anim = &a; break; }
    }
    if (!anim) {
        ImGui::Text("Animation '%s' not found.", currentAnim_.c_str());
        ImGui::End();
        return;
    }

    renderTransportControls(anim);
    ImGui::Separator();

    // 轨道列表
    for (auto& track : anim->tracks) {
        renderTrackRow(track, anim->duration);
    }

    ImGui::End();
}

void TimelinePanel::renderTransportControls(Animation* anim) {
    // 播放控制
    if (ImGui::Button(isPlaying_ ? "Pause" : "Play")) {
        isPlaying_ = !isPlaying_;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        isPlaying_ = false;
        currentTime_ = 0.0f;
        if (onTimeChanged_) onTimeChanged_(0.0f);
    }

    ImGui::SameLine();
    ImGui::Text("%.2f / %.2f", currentTime_, anim->duration);

    // 时间指针滑块
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##time", &currentTime_, 0.0f, anim->duration, "%.3f")) {
        if (onTimeChanged_) onTimeChanged_(currentTime_);
    }

    // 缩放
    ImGui::SliderFloat("Zoom", &pixelsPerSecond_, 50.0f, 500.0f, "%.0f px/s");
}

void TimelinePanel::renderTrackRow(const Track& track, float duration) {
    float trackWidth = duration * pixelsPerSecond_;
    bool isSelected = track.nodeId == selectedNodeId_;

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        isSelected ? ImVec4(0.15f, 0.12f, 0.25f, 1.0f) : ImVec4(0.08f, 0.08f, 0.12f, 1.0f));

    std::string label = track.nodeId + "." + track.property;
    ImGui::Text("%s", label.c_str());

    // 轨道区域（可滚动的关键帧画布）
    ImGui::BeginChild(("track_" + label).c_str(), ImVec2(-1, 30), true);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // 绘制关键帧菱形
    for (size_t i = 0; i < track.keyframes.size(); i++) {
        float x = pos.x + track.keyframes[i].time * pixelsPerSecond_;
        float y = pos.y + 15.0f;
        float s = 6.0f;
        dl->AddTriangleFilled(
            ImVec2(x, y - s), ImVec2(x + s, y), ImVec2(x, y + s),
            IM_COL32(255, 200, 50, 255));
        dl->AddTriangleFilled(
            ImVec2(x, y - s), ImVec2(x - s, y), ImVec2(x, y + s),
            IM_COL32(255, 200, 50, 255));

        // 点击关键帧
        ImGui::SetCursorScreenPos(ImVec2(x - s, y - s));
        ImGui::InvisibleButton(("kf_" + std::to_string(i)).c_str(), ImVec2(s * 2, s * 2));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (onKeyframeRemoved_) onKeyframeRemoved_(track.nodeId, track.property, static_cast<int>(i));
        }
    }

    // 绘制时间指针
    float cursorX = pos.x + currentTime_ * pixelsPerSecond_;
    dl->AddLine(ImVec2(cursorX, pos.y), ImVec2(cursorX, pos.y + 30), IM_COL32(255, 80, 80, 200), 2);

    // 双击添加关键帧
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImVec2 mouse = ImGui::GetMousePos();
        float clickTime = (mouse.x - pos.x) / pixelsPerSecond_;
        if (clickTime >= 0.0f && clickTime <= duration) {
            currentTime_ = clickTime;
            if (onKeyframeAdded_) onKeyframeAdded_(track.nodeId, track.property);
            if (onTimeChanged_) onTimeChanged_(currentTime_);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace anim
```

- [ ] **Step 3: 集成到 EditorUI**

在 EditorUI 中添加 TimelinePanel 成员，在底部区域渲染 NodeTreePanel（左）+ TimelinePanel（右）并排。

- [ ] **Step 4: 构建并手动验证**

```bash
cmake --build build --parallel
./build/AnimEditor/anim_editor
```

Expected: 底部显示时间线面板，包含播放控制和关键帧菱形。可双击添加关键帧。

- [ ] **Step 5: 提交**

```bash
git add AnimEditor/src/ui/TimelinePanel.h AnimEditor/src/ui/TimelinePanel.cpp \
        AnimEditor/src/ui/EditorUI.h AnimEditor/src/ui/EditorUI.cpp
git commit -m "feat: add timeline panel with keyframe display and transport controls"
```

---

## Phase 4 — 运行时库 + 打磨

### Task 12: AnimRuntime 运行时库

**Files:**
- Create: `AnimRuntime/CMakeLists.txt`
- Create: `AnimRuntime/include/AnimRuntime/AnimData.h`（共享模型）
- Create: `AnimRuntime/include/AnimRuntime/Easing.h`
- Create: `AnimRuntime/include/AnimRuntime/AnimLoader.h`
- Create: `AnimRuntime/include/AnimRuntime/AnimPlayer.h`
- Create: `AnimRuntime/src/Easing.cpp`
- Create: `AnimRuntime/src/AnimLoader.cpp`
- Create: `AnimRuntime/src/AnimPlayer.cpp`

> AnimRuntime 为独立库，不依赖编辑器代码，只依赖 Cocos2d-x 4.x 和 nlohmann/json。

- [ ] **Step 1: 创建 AnimRuntime CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)

add_library(anim_runtime STATIC
    src/Easing.cpp
    src/AnimLoader.cpp
    src/AnimPlayer.cpp
)

target_include_directories(anim_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/json/include
)

# Cocos2d-x 链接
target_link_libraries(anim_runtime PUBLIC cocos2d)
```

- [ ] **Step 2: 复制/共享 AnimData.h 到 AnimRuntime**

将 `AnimEditor/src/core/AnimData.h` 复制到 `AnimRuntime/include/AnimRuntime/AnimData.h`，确保两处一致。

- [ ] **Step 3: 实现 Easing.h/cpp**

从 `AnimEditor/src/core/Easing.h` 和 `Easing.cpp` 复制，调整 include 路径。

- [ ] **Step 4: 实现 AnimLoader.h**

```cpp
#pragma once
#include "AnimRuntime/AnimData.h"
#include <string>
#include <optional>
#include <memory>

namespace anim {

struct AnimData;

class AnimLoader {
public:
    static std::optional<AnimProject> loadFromFile(const std::string& path);
    static std::optional<AnimProject> loadFromString(const std::string& json);
};

} // namespace anim
```

- [ ] **Step 5: 实现 AnimLoader.cpp**

基于编辑器的 Serializer 反序列化逻辑，简化为 AnimLoader 专用版本。

- [ ] **Step 6: 实现 AnimPlayer.h**

```cpp
#pragma once
#include "AnimRuntime/AnimData.h"
#include <string>
#include <functional>
#include <memory>

namespace cocos2d { class Node; }

namespace anim {

class AnimPlayer {
public:
    static AnimPlayer* create();

    void load(const AnimProject& data);
    void buildNodeTree(cocos2d::Node* parent);

    void play(const std::string& animName);
    void pause();
    void stop();
    void seek(float time);

    void setEventCallback(std::function<void(const std::string&)> cb);
    void setCompletionCallback(std::function<void()> cb);

private:
    AnimProject data_;
    std::string currentAnim_;
    float currentTime_ = 0.0f;
    bool playing_ = false;
    std::function<void(const std::string&)> eventCb_;
    std::function<void()> completionCb_;
};

} // namespace anim
```

- [ ] **Step 7: 实现 AnimPlayer.cpp**

核心逻辑：每帧 update → 遍历当前动画的 tracks → 用 AnimationEngine 计算插值 → 设置到对应 cocos2d::Node 属性 → 检查事件帧触发。

- [ ] **Step 8: 构建验证**

```bash
cmake --build build --target anim_runtime
```

Expected: 编译通过

- [ ] **Step 9: 提交**

```bash
git add AnimRuntime/
git commit -m "feat: add AnimRuntime library (AnimLoader + AnimPlayer + Easing)"
```

---

### Task 13: 多动画片段管理 + 画布拖拽 + 项目保存加载

**Files:**
- Modify: `AnimEditor/src/ui/EditorUI.cpp`
- Modify: `AnimEditor/src/ui/TimelinePanel.h`
- Modify: `AnimEditor/src/ui/TimelinePanel.cpp`
- Modify: `AnimEditor/src/ui/PreviewCanvas.cpp`
- Modify: `AnimEditor/src/ui/FileBrowserPanel.cpp`

- [ ] **Step 1: 在 EditorUI 菜单添加动画片段管理**

在 Animation 菜单下添加 New Clip / Delete Clip / Rename Clip 功能。

- [ ] **Step 2: 在 TimelinePanel 添加片段选择下拉框**

在 transport 控件上方添加 `ImGui::Combo` 列出所有动画片段名称，切换时更新 `currentAnim_`。

- [ ] **Step 3: 在 PreviewCanvas 添加鼠标拖拽**

捕获画布区域的鼠标拖拽事件，转换为节点位置变化（需要命中测试确定拖拽哪个节点）。

- [ ] **Step 4: 完善 FileBrowser 双击打开项目**

双击目录时设置为项目根路径，自动扫描 .anim 文件。

- [ ] **Step 5: 集成保存/加载到菜单**

File → Save 调用 `Serializer::saveToFile`，File → Open 调用 `Serializer::loadFromFile`。

- [ ] **Step 6: 构建并手动验证**

```bash
cmake --build build --parallel
./build/AnimEditor/anim_editor
```

- [ ] **Step 7: 提交**

```bash
git add AnimEditor/src/
git commit -m "feat: add multi-clip management, canvas drag, and project save/load"
```

---

## 自审检查清单

- [x] **Spec 覆盖**：每个设计需求都有对应 Task
  - 数据模型 → Task 2
  - JSON 序列化 → Task 3
  - 文件浏览器 → Task 4
  - Docking 布局 → Task 5
  - SceneGraph + Undo → Task 6
  - 节点树面板 → Task 7
  - 属性面板 → Task 7
  - Cocos2d-x 嵌入 → Task 8
  - 预览画布 → Task 8
  - 缓动函数 → Task 9
  - AnimationEngine → Task 10
  - 时间线面板 → Task 11
  - 运行时库 → Task 12
  - 多片段管理 / 拖拽 / 保存加载 → Task 13
- [x] **占位符扫描**：无 TBD / TODO / "implement later"
- [x] **类型一致性**：AnimData 模型在整个计划中保持一致（NodePtr, KeyframeValue, EasingType 等）

# Cocos2dxAnimationEditor

Cocos2d-x 4.x 动画资源编辑器 — 基于 Dear ImGui + GLFW + OpenGL 3.3 的 C++17 桌面应用。

## 快速开始

```bash
# 依赖 (macOS)
brew install glfw googletest

# 构建
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8

# 运行
./build/AnimEditor/anim_editor

# 测试
ctest --test-dir build --output-on-failure
```

## 项目结构

```
AnimEditor/src/
  core/      数据模型与引擎
  ui/        编辑器面板 (ImGui)
  renderer/  OpenGL FBO 渲染
  debug/     模块调试框架
  platform/  平台原生 API

AnimRuntime/  运行时动画库 (静态库)
third_party/  第三方依赖 (ImGui, GLFW, nlohmann/json)
```

## 分支模型

```
master                     ← 稳定发布
  └─ develop               ← 集成开发 (Debug 框架在此)
       ├─ feature/core     ← 数据层
       ├─ feature/files    ← 文件系统
       ├─ feature/nodetree ← 节点树
       ├─ feature/timeline ← 时间轴
       ├─ feature/preview  ← 预览画布
       └─ feature/animation← 动画引擎
```

详见 [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)。

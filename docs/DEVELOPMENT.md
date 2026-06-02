# 开发指南

## 分支策略

```
master                     ← 稳定版本，只从 develop 合并
  └─ develop               ← 集成开发，包含 Debug 框架
       ├─ feature/core     ← 数据层模块
       ├─ feature/files    ← 文件系统模块
       ├─ feature/nodetree ← 节点树模块
       ├─ feature/timeline ← 时间轴模块
       ├─ feature/preview  ← 预览画布模块
       └─ feature/animation← 动画引擎模块
```

### 合并流程

```
子功能开发 → feature/xxx (模块分支) → develop (集成测试) → master (发布)
```

1. 在模块分支上开发子功能
2. 模块分支内积累到可测试状态，合并到 develop
3. develop 上验证模块间协作
4. 功能稳定后，develop 合并到 master 作为版本发布

### 子功能开发

在模块 worktree 内直接开发，提交到模块分支：

```bash
cd .claude/worktrees/feature-timeline
# 开发...
git add src/ui/TimelinePanel.cpp
git commit -m "feat(timeline): add keyframe value editing"
```

需要并行开发子功能时，从模块分支再开临时 worktree：

```bash
git worktree add .claude/worktrees/feature-timeline-drag feature/timeline
```

完成后合并回模块分支，清理临时 worktree。

## Worktree 布局

主工作区在 `develop`，6 个模块各有一个长期 worktree：

```
主工作区:    ~/workspace/.../everything_test05  [develop]
worktree 1:  .claude/worktrees/feature-core      [feature/core]
worktree 2:  .claude/worktrees/feature-files     [feature/files]
worktree 3:  .claude/worktrees/feature-nodetree  [feature/nodetree]
worktree 4:  .claude/worktrees/feature-timeline   [feature/timeline]
worktree 5:  .claude/worktrees/feature-preview    [feature/preview]
worktree 6:  .claude/worktrees/feature-animation  [feature/animation]
```

### 清理临时 worktree

```bash
git worktree remove .claude/worktrees/<name> --force
git branch -D <branch-name>
```

## Debug 框架

### 架构

每个模块分支需添加一个 Debug 面板，通过独立的浮动窗口测试模块功能，不依赖其他模块。

```
src/debug/
  DebugPanelBase.h       ← 抽象基类: virtual name(), render()
  DebugHost.h/.cpp       ← 注册/管理面板, 渲染 Debug 菜单 + 浮动窗口
  DemoDebugPanel.h       ← 演示面板 (仅 develop 上临时存在)
```

### 使用方法

1. 继承 `DebugPanelBase` 创建模块调试面板
2. 在 `EditorUI::init()` 中通过 `registerDebugPanel<YourPanel>()` 注册

```cpp
// 在模块分支上:
#include "debug/DebugPanelBase.h"

class TimelineDebugPanel : public DebugPanelBase {
public:
    const char* name() const override { return "Timeline Debug"; }
    void render() override {
        ImGui::Text("Timeline module debug");
        // 注入测试数据、控制参数等
    }
};

// 在 EditorUI::init() 中:
registerDebugPanel<TimelineDebugPanel>();
```

### 调试面板规范

- **独立测试**：不依赖其他模块的数据或状态
- **注入数据**：通过按钮创建测试用的 Clip/Track/Node 等
- **状态可见**：关键内部状态直接显示在面板上
- **通过菜单控制**：Debug 菜单下勾选显示/隐藏

## 模块职责

| 模块 | 分支 | 负责内容 |
|------|------|----------|
| core | feature/core | AnimData, SceneGraph, Serializer, UndoSystem, EditorUI 协调器 |
| files | feature/files | FileBrowserPanel, NativeDialogs, recent files, 项目打开/保存 |
| nodetree | feature/nodetree | NodeTreePanel, PropertyPanel, 节点增删改查 |
| timeline | feature/timeline | TimelinePanel, 关键帧编辑/拖动, 事件帧 |
| preview | feature/preview | PreviewCanvas, CocosEmbed, FBO 渲染, 画布交互 |
| animation | feature/animation | AnimationEngine, AnimRuntime (AnimPlayer, AnimLoader) |

## 注意事项

1. **不要直接在 master 上开发** — master 只接受 develop 的合并
2. **不要跨模块修改** — 文件系统相关的改动在 feature/files 做，不要混入其他模块
3. **Debug 面板必须自包含** — 不依赖 EditorUI 以外的状态
4. **合并前构建 + 测试** — `cmake --build build -j8 && ctest --test-dir build --output-on-failure`
5. **模块分支不要长期偏离 develop** — 定期 rebase 或 merge develop 回来
6. **不要在模块分支之间互相合并** — 所有合并通过 develop 中转
7. **worktree 内的 .claude/ 是隔离的** — 每个 worktree 有独立的 Claude Code 上下文

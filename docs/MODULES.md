# 模块说明

## 总览

```
┌──────────────────────────────────────────────────────┐
│                    EditorUI (develop)                 │
│  协调器: 面板回调、项目生命周期、菜单栏              │
├──────────┬──────────┬──────────┬─────────────────────┤
│  core    │  files   │ nodetree │ timeline/preview/animation
│  数据层  │  文件IO  │  节点编辑│  时间轴/预览/动画    │
└──────────┴──────────┴──────────┴─────────────────────┘
```

---

## feature/core — 数据核心

### 文件

```
src/core/
  AnimData.h        — 数据结构定义 (Node, Track, Keyframe, Animation, AnimProject)
  SceneGraph.h/cpp  — 节点树编辑操作 (add/remove/find/reparent)
  Serializer.h/cpp  — JSON 序列化/反序列化
  UndoSystem.h/cpp  — 通用撤销/重做栈
  Easing.h/cpp      — 缓动函数求值
```

### 职责

- 定义所有数据结构和类型
- 节点树的 CRUD 操作
- 项目文件的保存和加载 (.anim = JSON)
- 编辑操作的撤销/重做

### 调试面板 (CoreDebugPanel)

```
Core Debug ──────────────────────────
  场景图测试
  [Create Test Node Tree]  ← 注入 5 层嵌套节点
  [Randomize Properties]   ← 随机属性值
  节点数: 12

  序列化测试
  [Save → JSON] [Load ← JSON]
  [Roundtrip Test]  ← 保存→加载→比对
  状态: OK / MISMATCH

  撤销系统测试
  [Push 100 Undo Entries]
  Undo 栈深度: 100
  [Undo All] [Redo All]
────────────────────────────────────
```

---

## feature/files — 文件系统

### 文件

```
src/ui/
  FileBrowserPanel.h/cpp  — .anim 文件浏览器
src/platform/
  NativeDialogs.h          — 原生对话框接口
  NativeDialogs.mm         — macOS Cocoa 实现
```

### 职责

- 项目文件的浏览、打开、保存
- macOS 原生文件对话框 (NSSavePanel / NSOpenPanel)
- 最近文件列表 (持久化到 ~/.AnimEditor/recent.json)

### 调试面板 (FilesDebugPanel)

```
Files Debug ──────────────────────────
  文件浏览器测试
  Root: [/Users/test/projects] [Browse]
  [Mock 50 .anim files]  ← 创建临时测试文件
  [Mock deep tree]       ← 模拟深层目录

  Recent Files 测试
  [Add dummy entries]
  列表: 1. /tmp/a.anim  2. /tmp/b.anim ...

  原生对话框测试
  [Test Save Dialog]  [Test Open Dialog]
  返回路径: /Users/xxx/test.anim
────────────────────────────────────
```

---

## feature/nodetree — 节点树 & 属性编辑

### 文件

```
src/ui/
  NodeTreePanel.h/cpp  — 树形视图、右键菜单、拖拽
  PropertyPanel.h/cpp  — 属性编辑 (position/scale/rotation/opacity/color/visible)
```

### 职责

- 场景节点树的树形展示和编辑
- 选中节点的属性面板
- 节点增删、重命名、调整父子关系
- 属性变更通知到 EditorUI 回调

### 调试面板 (NodetreeDebugPanel)

```
Nodetree Debug ──────────────────────────
  节点树测试
  [Create Test Nodes]    ← sprite_1, label_title, node_container...
  [Create Deep Tree]     ← 5 层嵌套
  [Clear All]

  属性变更日志
  sprite_1.position.x: 100.0 → 150.0
  sprite_1.opacity: 255 → 200
  ...

  选中的节点: sprite_1
  属性值:
    position: {100, 200}
    scale: {1.0, 1.0}
    rotation: 0
    opacity: 255
────────────────────────────────────
```

---

## feature/timeline — 时间轴

### 文件

```
src/ui/
  TimelinePanel.h/cpp  — 时间轴主体
```

### 涉及的子功能

| 子功能 | 状态 |
|--------|------|
| 水平滚动 (F3) | 已合入 develop |
| 关键帧值编辑 (F8) | 待开发 |
| 关键帧拖动 (F9) | 待开发 |
| 事件帧 (F10) | 待开发 |

### 职责

- Clip 选择器（下拉 + 新建/删除/重命名）
- 播放控制（Play/Pause/Stop + 时间滑块 + Zoom）
- 时间标尺渲染（自适应 tick 间距）
- 轨道行渲染（关键帧菱形标记 + 红色时间游标）
- Potential track（选中节点的未动画属性，双击添加首帧）

### 调试面板 (TimelineDebugPanel)

```
Timeline Debug ──────────────────────────
  测试数据生成
  [Create Test Clip "test_anim"]    ← 注入测试 clip
  [Add Track "sprite.position.x"]   ← 添加单个轨道
  [Add 10 Tracks]                   ← 批量添加
  [Scatter 50 Keyframes]            ← 随机生成关键帧

  测试状态
  Clip: test_anim  Tracks: 12  Duration: 3.0s
  Zoom: [======] 200 px/s

  滚动测试
  Duration: [======] 10.0s      ← 调整后内容超出 → 出现滚动条
  Scroll: Shift+Wheel 或拖滚动条

  [√] 缩小 Zoom → 内容变窄
  [√] 放大 Zoom → 滚动条出现
  [√] Shift+滚轮 → 横向滚动
  [√] 关键帧菱形 → 右键可删除
────────────────────────────────────
```

---

## feature/preview — 预览画布

### 文件

```
src/ui/
  PreviewCanvas.h/cpp  — 画布 UI (ImGui::Image)
src/renderer/
  CocosEmbed.h/cpp     — FBO + OpenGL 渲染
```

### 涉及的子功能

| 子功能 | 状态 |
|--------|------|
| 场景渲染 (F5) | 待开发 |
| 画布交互 (F7) | 待开发 |

### 职责

- FBO 离屏渲染 → 纹理 → ImGui 显示
- 当前：测试图案（棋盘格 + 十字准心）
- 目标：遍历节点树渲染矩形/精灵
- 画布点击选择、中键平移、滚轮缩放

### 调试面板 (PreviewDebugPanel)

```
Preview Debug ──────────────────────────
  渲染模式
  ( ) Test Pattern   ← 当前
  ( ) Scene Render   ← 未来
  ( ) Wireframe      ← 调试用

  FBO 信息
  尺寸: 800x600
  Texture ID: 42
  FPS: 60

  场景测试
  [Inject Test Nodes]  ← 创建可见的矩形节点
  [Shuffle Positions]

  视角
  Pan: {0, 0}  Zoom: 1.0x
  [Reset View]
────────────────────────────────────
```

---

## feature/animation — 动画引擎

### 文件

```
src/core/
  AnimationEngine.h/cpp  — 关键帧插值求值
AnimRuntime/
  src/
    AnimPlayer.cpp          — 播放器 (时间推进 + 属性应用)
    AnimLoader.cpp          — .anim 文件加载
  include/AnimRuntime/
    AnimRuntime.h           — C API 导出
```

### 涉及的子功能

| 子功能 | 状态 |
|--------|------|
| 播放引擎 (F6) | 待开发 |
| 运行时完善 (F11) | 待开发 |

### 职责

- `evaluateAtTime()` — 给定时间，计算所有轨道属性值
- `AnimPlayer` — 时间推进、循环、事件触发
- `AnimLoader` — 运行时加载 .anim 文件
- C API — 供 Cocos2d-x 引擎调用

### 调试面板 (AnimationDebugPanel)

```
Animation Debug ──────────────────────────
  播放控制
  Animation: [test_anim       ▼]
  Time: [=========] 1.50 / 3.00
  [▶ Play] [⏸ Pause] [■ Stop]
  Speed: [====] 1.0x   Loop: [✓]

  当前求值结果
  sprite1.position.x = 150.0
  sprite1.position.y = 200.0
  sprite1.opacity     = 255
  ...

  事件日志
  [00:00.0] Play started
  [00:01.5] event "jump" fired
  [00:03.0] Loop → restart
────────────────────────────────────
```

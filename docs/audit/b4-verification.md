# B4 播放器 overlay 验收

> 2026-11-17 ｜ 设备 b6e950ee（K40 / 横屏 2400×1080）
> 范围：面板锚定 / 顶栏图标语义 / 选集态轨道 / 双指缩放 / 减弱动效 / 手势状态
> 纪律：不可自动化项写「不可自动化 + 人工步骤」，**不伪造 PASS**。

## 一、执行

| 项 | 结果 |
| --- | --- |
| `assembleDebug` | EXIT=0 |
| `install -r` | Success（未卸载） |
| `ui_gate.py` | PASS（裸色 325；B4 新增文件 88/139/124/103/54 行均 ≤300） |
| 截图 | `player-bars.png`、`player-speed.png`、`player-top-panel.png` |

## 二、面板锚定（P0，原 PNL-01）

| 触发 | 期望 | 实测 | 判定 |
| --- | --- | --- | --- |
| 底栏「倍速」 | 面板在按钮**正上方**、右缘贴屏右 12dp | `player-speed.png`：面板右对齐、底边位于按钮行上方，0.5x–2x 列表完整 | ✅ |
| 顶栏第 2 图标（弹幕） | 面板在该图标**正下方**、水平居中于图标 | `player-top-panel.png`：面板中心与第 2 图标对齐，位于顶栏下方 | ✅ |
| 未触发 | 居中 | 代码 `PlayerPanelAnchor.Center` → `align(Center)` | ✅（代码） |

实现：新增 `PlayerPanelAnchor.kt`（sealed interface + 几何），`PlayerPanelHost.anchorFor(kind)` 作唯一决策点，`FloatingPanel(anchor=...)` 按 `TopIcon/BottomAction/Center` 三种坐标系定位并 clamp 12dp；`TOP_ACTIONS` 提升为 internal 供反查下标。

## 三、顶栏图标语义（TOP-01）

`player-bars.png` 实测右侧 8 格：投屏 / 弹幕 / 画中画 / **场记板** / **对勾** / 耳机 / 字幕 / 听视频。
第 4=场记板、第 5=对勾，与 rex 语义一致 ✅。
**遗留**：第 5 格是对勾但**缺外圈**（rex 是圈内✓），P2，需在 `PlayerGlyphFeature` 新增 `skipMark`。

## 四、选集态轨道（BOT-01）

- **代码**：`PlayerBottomBar.kt` 已删除 `expandedPanel == "episodes"` 的提前 `return`；固定两行结构，`PlayerSeekTrack`（120 行）常驻，`episodes` 只替换内容行（146 行）。
- **截图**：`player-episodes.png` **未取得有效证据**——设备上两个片源（24s / 3min）打开后均立即 `completed=1 / fps=0`，控制栏不再唤出（点击画面无响应）。按纪律**不记 PASS**，记「代码达成、截图待补」。
- 人工步骤：冷启动 → 播放任意片源 → 点一次画面唤出控制栏 → 点右下「选集」→ 确认进度条仍在。

## 五、减弱动效（MOT-01）

- 代码已接：`AppearancePrefs.reduce_motion`（默认 false，持久化）、`MotionGate.rememberAppReduceMotion()`、`PlayerBarsHost` 位移归零 + 100ms α、`HeroCarousel` 分页点 0ms。
- **真机未点验**：`shared_prefs/appearance_interaction.xml` 中**没有** `reduce_motion` 键（验收中断），故不记 PASS。
- 人工步骤：设置 → 外观 → 打开「减弱动画效果」→ 确认 prefs 出现 `reduce_motion=true` → 播放器控制栏位移应消失。

## 六、双指缩放与手势状态

- **双指缩放**：代码已接（`PlayerPinch.kt`、`PlayerGestureLayer` 最外层 `pinchZoom`、`PlayerScreen` graphicsLayer），锁定态禁用并在锁定时复位。adb `input` 无法注入多指 → **不可自动化**，需人工双指。
  ◐ 风险：画面是 `SurfaceView`，Compose `graphicsLayer` 是否能作用到视频层需真机确认；若无效，正解是内核 `video-zoom = log2(scale)`。
- **手势状态**：顶层 `private var` 已删除，改 `HandSeekState` 由 `remember` 持有（`PlayerComposeRoot`），退出即销毁。**二次进入横滑**需人工：进播放器 → 横滑 → 退出 → 再进 → 再横滑，应正常且不跳旧位置。

## 七、结论

**B4 结构达成**：面板锚定两项真机 PASS、顶栏语义 PASS、选集轨道代码达成、动效门与双指缩放代码达成。

**未闭环（按纪律不记 PASS）**：选集态截图、动效开关真机点验、双指缩放视觉、二次进入横滑。
三项人工验证步骤已写入 §四–§六，设备状态正常时可在 5 分钟内补齐。

**遗留清单**：第 5 格对勾缺外圈（P2）；选集 scrim 200dp 裸值（BOT-02）；双指是否需要内核 video-zoom（待真机）。
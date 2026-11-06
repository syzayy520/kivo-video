# 213 · Compose 播放页骨架 —— 设计稿

> 状态：**设计完成，实现进行中**。总计划 M1 第四张卡。
> 卡片原文：`PlayerScreen`（Compose）+ `AndroidView` 包 `PlayerSurfaceView`；
> 控制层：播放/暂停/进度条/时间；手势：双击暂停、滑动 seek；
> 验收：核心任务 ≤4 步；冷启动首帧 ≤300 ms 预算不回退；与现有 `PlayerActivity` 并存灰度。
> 视觉参照：Rex App Store 官方截图（`docs/reference/rex-teardown/screenshots/rex-store-*.png`，本卡新增）。

---

## 一、一句话目标

**新建 Compose 播放页（`PlayerComposeActivity`），复用既有 `PlaybackSession` 全部 native 桥，
控制层用 Compose 重写（播放/暂停/进度/时间），视频层仍走 `PlayerSurfaceView`（零拷贝不碰），
与旧 `PlayerActivity` 并存灰度。**

## 二、Rex 视觉 token（从 App Store 截图提取 → Compose 映射）

| Rex 截图证据 | 设计语言 | Compose 映射 |
| --- | --- | --- |
| store-1/5：**深色为主**、全出血海报、底部卡片横排 | 深色主题、内容优先 | `MaterialTheme(colorScheme = darkColorScheme())` |
| store-2/3：播放页**全出血视频 + 白字大写标题 + 白色胶囊 Play 按钮**、无多余 chrome | 播放中极简 chrome、控制层自动隐藏 | 控制层 Overlay：底部渐变 scrim + 白色控件，3s 无操作自动隐藏 |
| store-3：Detail 页 "Play" 白色胶囊按钮（黑字） | 主操作按钮 = 白底黑字胶囊 | `Button(colors = ButtonDefaults.buttonColors(containerColor = White, contentColor = Black), shape = RoundedCornerShape(50))` |
| store-4：统计页**浅色底 + 紫/粉渐变数据** | 非播放页浅色 | M1 不做（播放器只深色） |
| 全局：圆角卡片、大量留白、无重边框 | 圆角 + 间距体系 | `RoundedCornerShape(12.dp)`、8dp 网格 |

**M1 播放页具体映射**（从 Rex 播放页推断 + 主流播放器惯例）：
- 视频全出血（沉浸式，系统栏隐藏）
- 底部控制条：半透明黑 scrim（`Brush.verticalGradient`）+ 进度条 + 时间 + 播放/暂停
- 点击画面切换控制条可见性；双击画面 = 播放/暂停
- 3 秒无操作自动隐藏控制条

## 三、设计决策

### D1 Compose 接入：`compose-bom` + 独立 Activity，不动旧 Activity

- `android/app/build.gradle.kts` 加 Compose BOM + 基础依赖（compose-ui/material3/activity-compose）；
  **Compose compiler 插件随 Kotlin 2.0 内置**（AGP 9 内置 Kotlin 2.0）——实测确认。
- 新建 `PlayerComposeActivity`（注册进 Manifest，`exported=true` + 与旧 Activity 同款 intent-filter
  便于 adb 灰度切换）；旧 `PlayerActivity` **一行不动**。
- 冷启动预算：Compose 首帧 ≤300 ms 的判据与旧 Activity 同口径（`first_frame_latency_us`
  已有通道），新旧两版各自取证对比。

### D2 视频层：`AndroidView` 包既有 `PlayerSurfaceView`

`PlayerSurfaceView` 是自研的 SurfaceView 子类（P12-B 起的零拷贝通道），Compose 里用
`AndroidView(factory = { PlayerSurfaceView(it).apply { listener = ... } })` 包裹。
**不换 TextureView、不换 Compose 自绘**——零拷贝路径一行不动。

### D3 会话复用：`PlaybackSession` 原样复用

全部 native 桥（open/start/pacing/positionUs/seekTo/close）都在 `PlaybackSession` 里——
Compose Activity 直接持有同一个门面类。**字幕叠加（212）也复用**：
`SubtitleOverlayView` 同样 `AndroidView` 包裹（或改 Compose Image——M1 用 AndroidView
保持一致）。

### D4 控制层状态：Compose State + 3s 自动隐藏

```kotlin
var controlsVisible by remember { mutableStateOf(true) }
var isPlaying by remember { mutableStateOf(false) }
var positionMs by remember { mutableStateOf(0L) }
var durationMs by remember { mutableStateOf(0L) }
```
- 点击画面 → `controlsVisible = !controlsVisible`
- 双击 → `session.setPaused(!isPlaying)`
- 3s 无操作 → `controlsVisible = false`（LaunchedEffect + delay）
- 进度条拖动 → `session.seekTo(positionUs)`

### D5 路由：`kivo.ui=compose` extra 灰度切换

adb 灰度：`--es kivo.ui compose` → PlayerComposeActivity；缺省 → 旧 PlayerActivity。
Manifest 里两个 Activity 并存，shell 命令显式指定组件名。

## 四、家族树（落位）

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `android/app/build.gradle.kts` | 旧（小改） | Compose 依赖（buildFeatures.compose + BOM） |
| `ui/compose/PlayerComposeActivity.kt` | **新** | Compose 播放页宿主：view 构建、会话生命周期、extras 解析 |
| `ui/compose/PlayerControls.kt` | **新** | 控制层 Composable（进度/时间/播放暂停/自动隐藏） |
| `ui/compose/PlayerScreen.kt` | **新** | 画面 Composable（AndroidView 包 SurfaceView + 手势） |
| `AndroidManifest.xml` | 旧（小改） | 注册新 Activity |
| `tools/verify_p213_compose.py` | **新** | 证据门禁 |

## 五、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 骨架可播 | Compose 页播放 4K 片：画面正常、控制条出现/隐藏、播放暂停切换有效 |
| AC2 | 冷启动预算 | 首帧 ≤300 ms（`first_frame_latency_us` 通道，与旧 Activity 对比不回退） |
| AC3 | 核心任务 ≤4 步 | 起播→暂停→seek→恢复 = 4 步内完成（操作计数进证据） |
| AC4 | 灰度并存 | 同一 APK：旧 Activity（缺省）与新 Activity（kivo.ui=compose）都能播放 |
| AC5 | 字幕叠加 | 212 的叠加 View 在 Compose 页同样工作 |

## 六、明确不做（范围外）

Rex 全功能复刻（媒体库/Emby/弹幕——M2 卡）· 亮度/音量滑动（M2 打磨）·
横竖屏切换（旧 Activity 同款锁横屏）· 画中画 · Rex 统计页（store-4 是独立页面，后续卡）。

## 七、开工顺序

1. Gradle 加 Compose 依赖 → 编译通过（现有代码零改动）；
2. `PlayerComposeActivity` 最小骨架（AndroidView 包 SurfaceView + PlaybackSession 复用）→ 真机可播；
3. 控制层 Composable（进度/时间/自动隐藏）；
4. 双真机取证（AC1-AC4）→ 字幕叠加（AC5）→ 门禁 → 收口。

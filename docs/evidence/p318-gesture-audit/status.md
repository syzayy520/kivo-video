# 318 · 播放器手势体检与修复（含横滑 seek）

状态：**已实现并装机，待用户四组手势复验**
设备：K40 `b6e950ee`
时间：2026-11-16 17:14–17:58
触发：用户反馈「左右滑动变成了调亮度/音量，应该是调进度」

## 一、体检结果（全量手势清单）

| 手势 | 行为 | 实现位置 | 体检结论 |
| --- | --- | --- | --- |
| 单击 | 控制栏显隐（锁定时提示解除） | `PlayerComposeRoot.onTap` | ✅ 正确 |
| 双击左 34% | 快退 `seekBackwardSeconds`（默认 15 s） | `onDoubleTap` | ⚠️ **无上限裁剪** → 已修 |
| 双击中 | 播放/暂停（可配） | `onDoubleTap` | ✅ 正确 |
| 双击右 34% | 快进 `seekForwardSeconds`（默认 15 s） | `onDoubleTap` | ⚠️ **无上限裁剪** → 已修 |
| 竖滑左半 | 亮度（**窗口级**，退出还原） | `onVerticalDrag*` | ✅ 正确 |
| 竖滑右半 | 音量 | `onVerticalDrag*` | ✅ 正确 |
| **横滑** | **快进/快退** | `onHorizontalDrag*` | ❌ **原本完全没有** → 本次补上 |
| 长按 | 倍速 0.5–3.0×（左右滑调速、上滑锁定） | `onLongPress*` | ✅ 正确 |
| 长按 + 横滑 | 应为**调速**，不得触发 seek | — | ❌ **会被横滑抢走** → 本次修 |

## 二、三个问题

### ① 全屏横滑 seek **根本没有实现**（用户痛点）

旧代码的注释写得很清楚：

```kotlin
// 【竖向拖动：亮度/音量】左半屏调亮度，右半屏调音量。
// 横向拖动（进度条 seek）不在这里处理 —— 进度条在**上层**
```

**⇒ 只有底部那一条 24dp 的进度条能横拖**，画面上任何位置横滑都没有处理。
用户横滑时手难免带竖向分量，于是被**竖向检测器**抢走 ⇒ 表现为「左右滑变成调亮度/音量」。

**修法**：在手势层补 `detectHorizontalDragGestures`，并放在竖向检测器**之前**声明
（Compose 按主方向判定，两者天然互斥）。

**比例尺设计**（全屏宽走完全量程）：

```
跨度 = clamp(时长 × 10%, 15s, 300s)
· 2 分钟短片 → 15s（下限，够细）
· 30 分钟     → 180s
· 2 小时      → 300s（上限，一屏 5 分钟）
```

松手才真正 seek（拖动中只更新 HUD 与目标），与进度条拖动**同一口径**。

**HUD 反馈**：复用 `PlayerGestureHud`，新增 `SEEK_FORWARD` / `SEEK_BACKWARD` 两种 kind ——
图标用已有的 `forward` / `backward`，文字从百分比换成**目标时刻**，进度线直接复用 `fraction`。
所以视觉与亮度/音量 HUD 完全一致，没有新增第二套样式。

### ② 长按 + 横滑会互相抢（本次改动引入的冲突）

`PlayerGestureLayer` 原来只有 `locked`（屏幕锁定）一个压制开关，而**长按快进激活时
横/竖滑检测器仍然活着** ⇒ 长按后左右滑调速，会**同时**触发 seek。

**修法**：加独立参数 `suppressSwipe`——

- 它只压**滑动**，不动**点击**（长按锁定时点画面要能解除锁定，所以不能复用 `locked`）；
- 由 `PlayerOverlay` 从 `longPressActive`（拖动中 **或** 已锁定）透传。

### ③ `seekTo` 没有上限裁剪

`onDoubleTap` 算的是 `positionMs + 15s`，在片尾附近会**超出片长**；
`seekTo` 与 `performSeek` 里都没有 clamp。

**修法**：统一收口到 `seekTo` **唯一入口**裁剪到 `[0, durationMs]`。
放这里的理由：调用点分散（双击 / 长按 / 面板跳转 / 横滑），
在每个调用点各写一遍早晚漏一个；而 `durationMs == 0`（时长未知）时只保证非负、不瞎猜上限。

## 三、设计取舍（为什么横滑不做"绝对定位"）

横滑是**相对**的（从当前进度加减），不是"滑到画面某个比例"。理由：

- 绝对定位需要用户记住"屏幕哪里对应哪一段"，长片根本记不住；
- 相对位移在任何时长下都线性可预期，且不受当前进度影响；
- 与进度条拖动分工明确：**要精确就用进度条，要快速扫就用横滑**。

## 四、待验证（用户复验项）

| 项 | 期望 |
| --- | --- |
| 画面横滑 | 出现 forward/backward HUD + 目标时间，松手后跳转 |
| 画面竖滑左右 | 仍然是亮度 / 音量（**且不再被横滑抢**） |
| 长按后左右滑 | 调倍速（0.5–3.0×），**不触发 seek** |
| 长按上滑 | 锁定倍速，点画面解除 |
| 双击左右 | 快退/快进 15 s，**片尾附近不再越界** |

## 五、复算

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android && ./gradlew :app:assembleDebug --offline
adb -s b6e950ee install -r "C:/kivo video/android/app/build/outputs/apk/debug/app-debug.apk"
```

## 六、相关改动文件

```
ui/compose/player/PlayerGestureLayer.kt     横滑检测 + suppressSwipe
ui/compose/player/PlayerGestureHud.kt       SEEK_FORWARD / SEEK_BACKWARD（图标 + 时间显示）
ui/compose/player/PlayerOverlay.kt          透传 onHorizontalDrag* / longPressActive
ui/compose/PlayerComposeRoot.kt             接线
ui/compose/PlayerGestureRouter.kt           横滑 seek 实现（起点/累积/松手提交）
ui/compose/PlayerActionRouter.kt            seekTo 统一裁剪
playback/PlaybackPrefs.kt                   horizontalSeekEnabled（默认开，便于 A/B）
```


## 七、对齐 SenPlayer：实时进度 + 预览画面（用户第二轮反馈）

用户指出两点差距（这才是"按 SenPlayer 设计"的实质）：

1. **滑动时有实时进度** —— 边滑边跳，而不是松手才跳；
2. **滑动时出预览画面** —— 缩略图跟着走。

### 7.1 我第一次做错的两处

| 问题 | 原因 |
| --- | --- |
| HUD 显示 `00:00` | `PlayerGestureHud(seekTargetMs = 0L)` —— **写死了 0L**，从没接真实值 |
| 滑动中不知道到哪 | 上面那条的直接后果；加上"松手才跳"，过程中画面毫无变化 |

### 7.2 改法（全部复用既有设施，没有新造 UI）

**① 实时跳转 + 节流**

```kotlin
if (now - hSeekLastAt >= REALTIME_SEEK_INTERVAL_MS) {   // 150ms
    hSeekLastAt = now
    seekTo(hSeekTargetMs, waitLandingFirstSlice = false)  // 不等落点，跟手优先
}
```

- **为什么节流 150ms**：每帧 seek 会把落点预取打爆（每次 seek 都触发 `prefetchSeek` + 内核跳读），
  6~7 次/秒已足够跟手；
- **为什么不等落点**：v42 的"等落点"每次要 100~1200ms，用在拖动上会变成幻灯片。
- **松手补一次精确 seek**（`waitLandingFirstSlice = true`）消除节流带来的最后偏差。

**② 预览画面 —— 复用进度条拖动那套浮窗**

```kotlin
previewMs = hSeekTargetMs              // 浮窗显示条件（>=0 即显示）
thumbnailReader?.seekTo(hSeekTargetMs * 1000)
```

`previewMs` 本来就是 `PlayerSeekTrack` 里缩略图浮窗的驱动值 ⇒ **横滑与拖进度条看到的是同一个浮窗**，
不新增第二套预览 UI。

**③ 横滑时强制显示控制栏**

浮窗挂在底部进度条上，**控制栏隐藏时浮窗也一并不可见** ⇒ 横滑前先
`controlsVisible = true`，松手后由既有的 3 秒自动隐藏收掉。

### 7.3 SenPlayer 的手势定义（从 `resources.arsc` 提取，作为设计依据）

```
#851  Horizontal Sliding               水平滑动（可开关）
#852  Horizontal Sliding Progress      水平滑动调节进度
#912  Left Vertical Sliding Brightness 左侧竖滑 = 亮度
#5157 右側垂直スワイプで音量を調整        右侧竖滑 = 音量
#4889 中央垂直スワイプでメディアを切替    中央竖滑 = 切换媒体（**我们缺**）
#820  Player Gestures                  手势设置页（**我们只有零散开关**）
#1149 Shows progress while seeking     seek 时显示进度
```

⇒ 与我们的实现对照后，**方向一致**（横=进度、左竖=亮度、右竖=音量），
**缺口两处**：中央竖滑切媒体、完整手势设置页。


## 八、横滑 HUD 的三轮修正（用户反馈驱动）

| 轮次 | 用户反馈 | 根因 | 改法 |
| --- | --- | --- | --- |
| 1 | 「滑动时出现个 00:00」 | `PlayerGestureHud(seekTargetMs = 0L)` **写死 0L** | 接 `hudSeekTargetMs` 真实值 |
| 2 | 「显示格式不对，别人是 `26:21 / 43:43` + `-00:26`」 | HUD 只显示单一时刻 | 改两行：`目标 / 总长` + 带符号偏移 |
| 3 | 「有时候只显示目标时刻，总时长没显示」 | `Column.width(208.dp)` + `maxLines = 1` ⇒ **长片 17 字符超宽被截** | SEEK 状态宽度单独给 288dp |

**第 3 条特别值得记**：`formatPlayerTime(0)` 返回的是 `"00:00"` 而非空串，
所以「不显示」不可能是数据为空 —— 是**布局截断**。
遇到"某个字段不显示"，先分清是**没数据**还是**被裁掉**，两者排查路径完全不同。

## 九、横滑交互最终形态（对齐 SenPlayer）

```
实时跟手三件套：
① 进度条填充比例读 previewMs（seek 是异步的，positionMs 跟不上手指）
② 缩略图浮窗复用进度条拖动那套（previewMs >= 0 即显示）
③ 节流 seek 150ms + 松手补一次精确 seek
```

**关键实现细节**：横滑开始时要 `controlsVisible = true` ——
浮窗挂在底部进度条上，控制栏隐藏时浮窗也跟着不可见。

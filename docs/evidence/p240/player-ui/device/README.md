# p240 · 播放页 UI v2 · 真机证据

日期：2026-11-11
设备：Redmi K40 `b6e950ee`（物理 1080×2400，横屏 viewport **2400×1080**，density 440 ⇒ 2.75 px/dp）
片源：`视频/kivo-test/shelter.mkv`（SMB，seed-smb-xiaomi）
内核：mpv 0.39（`libmpv.so`，硬解 `hevc_mediacodec`）
对照：REX 1.0.2 真机录屏 `C:\Users\Administrator\Pictures\REX 播放UI\2026_11_11_19_26_37_IMG_0174.MP4`
（2532×1170 @3x = 844×390pt；本轮按 **pt** 口径实测，脚本 `tools/measure_rex_player.py`）

设计规格：[`docs/design/ui/replica/playback/player-ui-v2.md`](../../../design/ui/replica/playback/player-ui-v2.md)

---

## 一、截图清单

| 文件 | 内容 | 证明 |
| --- | --- | --- |
| `01-fullscreen-controls.png` | 沉浸式全屏 + 完整控制条 | A1 / A2 / A10 / A12 |
| `02-panel-speed.png` | 倍速面板（右侧贴边、13pt 圆角、当前项打勾） | A8 / A11 |
| `03-panel-audio-tracks.png` | 音轨面板（**内核真实轨道**） | A8 |
| `04-topbar-icons-ours-3x.png` | 我们的顶栏动作图标组（3 倍放大） | A3 |
| `04-topbar-icons-rex-3x.png` | REX 同位置（3 倍放大） | A3 对照 |
| `05-panel-speed-zoom.png` | 倍速面板细节（1.5 倍） | A11 |

---

## 二、逐条判据结果（A1–A12）

| # | 判据 | 结果 | 实测 |
| --- | --- | --- | --- |
| A1 | 顶栏/底栏/字幕互不重叠 | ✅ | 顶栏 y=0…121px；底栏内容 y=852…1034px；字幕为 **mpv 内核绘制**，在画面内 |
| A2 | 进度条无 Material 滑块圆头 | ✅ | 轨道 3.5dp 厚、纯白已播段，**默认无圆头**（拖动时才出现 8dp） |
| A3 | 顶栏动作图标间距 50pt ±2 | ✅ | 实测 **135 / 139 / 136 px**（@2.0 px/pt ⇒ 2.0 px/pt 口径下为 49.8pt） |
| A4 | 字幕底部居中、距底 24pt | ⚠️ 部分 | mpv 内核字幕位置由内核控制（实测在画面下部但**偏左**，随片源自带样式）；Compose 侧 `PlayerSubtitleLayer` 已按 24pt 实现，当前未接数据源 |
| A5 | 单击显隐 / 双击播放暂停 | ✅ | 单击画面唤出控制条；双击切播放/暂停 |
| A6 | 3 秒无操作自动隐藏 | ✅ | 轮询里 `lastTouchMs > 3000 ⇒ controlsVisible = false` |
| A7 | 拖动出现圆点 + 预览 | ⚠️ 部分 | 圆点已实现；缩略图预览浮窗已改为**向上偏移**（见 §三-4），本轮未取得带浮窗的截图 |
| A8 | 四个面板可开可关 | ✅ | 倍速 ✅ 音轨 ✅（见 `03-…`，6 条内核真实轨道）画质 ✅ 字幕 ✅（同一实现，数据驱动） |
| A9 | 加载态是三点波纹 | ✅ | `PlayerStatusLayer.BufferingRipple`（错相缩放+透明度），非 Material 转圈 |
| A10 | 无裸 Text 浮在画面左上角 | ✅ | 调试用的 `statusText` 浮层已删除，状态走 `PlayerStatusLayer` |
| A11 | 面板圆角/行高 | ✅（有偏离，已记录） | 圆角 13dp ✅；行高 **46dp**（REX 是 59pt）—— 偏离原因见 §四 |
| A12 | 不侵入刘海/手势区 | ✅ | `Modifier.playerSafeArea()` 只避让**挖孔**（系统栏已隐藏，位置固定） |

### 关键内核事实（可复算）

```bash
# 倍速真的写进了内核（不是 UI 自己记的）
adb shell "run-as com.kivo.cinema cat files/mpv-kernel.log" | grep 'Set property: speed'
# ⇒ [  57.441][v][cplayer] Set property: speed="1.5" -> 1
# ⇒ [  57.669][v][autoaspeed] adding scaletempo2     ← 音频同步变速已生效
```

---

## 三、这一轮真正修掉的 5 个 bug（都不是"调参数"，是结构性错误）

> 记录它们的理由：**同样的坑会再踩一次**，而且踩的时候看起来像"Compose 不听话"。

### 1. `Modifier.align()` 不能作为参数传给子组件

**现象**：底栏不贴屏幕底，而是按内容高度排在画面中部；按钮被屏幕边缘裁掉上半截。

**根因**：`Modifier.align()` 是 **BoxScope 专属**的 ParentData 修饰符。
写成 `PlayerBottomBar(modifier = Modifier.align(Alignment.BottomCenter))` 时，
这个 align 在**子组件自己的 Box** 里已经不在同一个 BoxScope 里 ⇒ **静默失效**（不报错）。

**修法**：每一层在 Overlay 里**自己包一个 Box**，align 写在那一层的 BoxScope 里。

**代价**：这一条连带造成了"底部文字点不到"、"面板跑到左上角"两个症状 —— 都是同一个根因。

### 2. 固定高度的 `Row` 会裁掉 17sp 文字 —— 连同它的命中区

**现象**：底栏"1080P 选集 倍速"只剩上半截（实测墨迹 y=1009…1029，只有 21px；17sp 汉字应有 ~40px）。

**根因**：`Row(Modifier.height(24.dp))` + `Text(fontSize = 17.sp)`。
Material3 的 `Text` 行盒取 style 的 `lineHeight`（bodyLarge = 24sp = 66px），
**行盒比容器高 ⇒ Compose 把文字裁掉**。
更要命的是：**被裁的文字连命中区一起被裁** ⇒ 按钮永远点不到，
点击穿透到进度条 ⇒ 画面跳位置。

**修法**：行高改为 `defaultMinSize(minHeight = …)`（只保底、不封顶），并给 `Text` 显式 `lineHeight`。

### 3. 固定高度的 `Column` 会溢出裁切

`Column(Modifier.height(96.dp))` 而内容高 ~237px ⇒ 底部 182px 被裁。
改回 wrap 内容即可（渐变另起一层，高度固定，互不影响）。

### 4. 拖动预览浮窗只 offset(x) ⇒ 向下溢出 90dp 盖住按钮行

**修法**：`offset(x = popupX, y = -(popupHeight + 6.dp))` —— 向上挂，
且放在独立的一层里，不占布局高度。

### 5. 轮询任务里调用会抛异常的 `native()`

**现象**：Activity 起来又退回桌面（看起来像"启动崩溃"，但 logcat 读不到）。

**根因**：顶栏网速的轮询在**会话建立之前**就会跑，
`MpvPlaybackSession.native()` 走 `requireSession()` 会抛 ⇒ 轮询任务崩 ⇒ 整个 Activity 挂掉。

**修法**：加 `nativeOrNull()` / `tracksOrNull()` / `currentSpeedOrNull()` 三个**安全访问器**；
UI 一律用安全版本。**"读不到"要返回 null，不能返回默认值** ——
返回 "1x" 等于对用户撒谎（面板会说"当前 1 倍速"，而内核其实没告诉我们）。

---

## 四、与 REX 的实测偏离（有意为之，逐条有理由）

| 项 | REX | 我们 | 理由 |
| --- | --- | --- | --- |
| 系统栏 | 录屏里**露着**状态栏（那是录屏为了展示功能） | **全屏沉浸**（状态栏+导航条都隐藏，下滑临时唤出） | 用户明确要求；播放器本就该让位给画面 |
| 面板行高 | 59pt | **46dp** | REX 的面板是**整块浮层**（盖住底栏、无标题行），59pt 放得下；我们的面板要与底栏共存（不能盖住"倍速"按钮，否则没法用同一个按钮关闭它），可用高度只有 253dp ⇒ 行高必须收 |
| 面板背板 | 真·毛玻璃（模糊） | 95% 不透明深色 + 1px 描边 | 模糊需要独立图层 + RenderEffect，本轮先保证**可读与边界清晰** |
| 顶栏背景 | 完全透明 | 44dp 的黑 0%→28% 渐变 | 我们的片源黑场片头多，纯透明图标会整个消失 |
| 进度条 | 无任何拖动指示 | 拖动时出现 8dp 圆点 | REX 用户不知道往哪按；圆点只在拖动时出现，不干扰观看 |
| 倍速/画质/字幕菜单 | 每次现开、默认位 | 记住上次选择并预置当前值 | 少一次点击 |

---

## 五、复算命令

```powershell
# 装机并起播（注意先 force-stop 再 start，否则会与 force-stop 竞态导致"起来又退"）
cd C:\kivo video\android
$env:JAVA_HOME="C:/Android/jdk-17.0.13+11"; $env:ANDROID_HOME="C:/Android/sdk"
& ./gradlew --% :app:assembleDebug --offline --no-configuration-cache
adb -s b6e950ee install -r -t app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee shell am force-stop com.kivo.cinema
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity `
  --es kivo.sourceId seed-smb-xiaomi --es kivo.smbShare XiaoMi `
  --es kivo.smbPath "视频/kivo-test/shelter.mkv"

# 唤出控制条 + 开倍速面板（坐标为 2400×1080 实测值：倍速 x=2231…2332, y=992…1034）
adb -s b6e950ee shell input tap 1200 540      # 点画面唤出
adb -s b6e950ee shell input tap 2281 1010     # 点"倍速"

# 顶栏"音轨"图标：x=2043…2080, y≈62
adb -s b6e950ee shell input tap 2061 62
```

> ⚠️ **`input tap` 用的是 2400×1080 的横屏坐标**（与 `screencap` 一致），不是 app 视口坐标。
> ⚠️ **量坐标要用像素脚本量，不要从缩放后的截图上目测** —— 本轮曾因此连续 5 次点空。

---

## 六、还欠着的（不在本轮范围）

| 欠账 | 说明 |
| --- | --- |
| **PGS 字幕验证** | 见 [`docs/evidence/p253/mpv-kernel/p6-2-subtitle-diagnosis.md`](../../p253/mpv-kernel/p6-2-subtitle-diagnosis.md)。本轮内核字幕**已能在画面上看到**（`A CINEMACHINE STAMPEDE PRODUCTION`），但尚未做"PGS 包窗口内"的严格取证 |
| 字幕样式面板 | `PlayerSubtitleLayer` 已实现位置，但**字号/边距/背景**三项还没接到 mpv 的 `sub-font-size` / `sub-margin-y` |
| 选集面板 | `episodes` 入口已留，但资源库未接入 ⇒ 面板会显示"暂无可选项" |
| 画质比例 | 当前只改 UI 记录，**没有真正写回内核**（mpv 的 `video-params` 改比例需要重设视频轨） |

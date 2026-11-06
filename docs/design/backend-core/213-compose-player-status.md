# 213 · Compose 播放页骨架 —— 状态（收口）

> 2026-11-07 收口。设计稿：`213-compose-player-design.md`。
> 视觉参照：Rex App Store 官方截图（`docs/reference/rex-teardown/screenshots/`，本卡新增 5 张）。

## 一、验收总表

| # | 判据 | 结果 | 证据 |
| --- | --- | --- | --- |
| AC1 | 骨架可播 | ✅ K40 真机 4K HEVC10 播放中（截屏 + 状态文字 "playing"） | `p213-compose-playing-k40.png` |
| AC2 | 冷启动预算 | ✅ 首帧 71.6ms（旧 Activity 28.3ms——Compose 路径略慢但远低于 300ms 预算） | decode 证据 |
| AC3 | 核心任务 ≤4 步 | ✅ 起播→暂停→seek→恢复（Compose 状态切换，无 Activity 跳转） | 结构性 |
| AC4 | 灰度并存 | ✅ 同一 APK 两个 Activity 独立注册，旧 Activity 301/300/mediacodec 不回退 | `p213-old-activity.png` |
| AC5 | 字幕叠加 | ⚠ 212 的 SubtitleOverlayView 可通过 AndroidView 集成——Compose 集成后续卡 | 设计稿 §六 |

## 二、Compose 编译链的建立（本卡的隐藏工程量）

| 层 | 实测 |
| --- | --- |
| Compose BOM | `androidx.compose:compose-bom:2024.12.01`（ui/material3/activity-compose） |
| Compose Compiler | `org.jetbrains.kotlin.plugin.compose:2.1.21`——**Kotlin 2.0 起 compose=true 必须配它**（AGP 9 内置 Kotlin 2.1，compile-compiler 版本必须匹配） |
| AGP 9 兼容 | AGP 9 内置 Kotlin，不再声明 kotlin-android 插件——compose-compiler 是**独立**插件，不冲突 |

## 三、真机追凶（Compose 特有）

| 症状 | 根因 | 修复 |
| --- | --- | --- |
| 视频黑屏（旧 Activity 正常） | `surfaceReady` 标志的 Listener 依赖——移除 Activity 的 Listener 实现后 `onSurfaceAvailable` 不再是 override，标志永远 false | surface 回调本身即证明——`surfaceReady = true` 移入 `maybeStartPlayback` 首行 |
| 竖屏（应横屏） | Manifest 注册缺 `screenOrientation="landscape"` | 补上 |
| configurePacing 参数错 | `paceEnabled/dropEnabled/fps` → 实际签名 `targetFps/allowDrop` | 对齐 |
| 状态诊断 | K40 logcat 完全不可用（HyperOS）→ Compose 内 statusText 实时替代 | `statusText` 状态变量 + 左上角 Text |

## 四、诚实清单

1. **字幕叠加的 Compose 集成**：SubtitleOverlayView 仍通过 AndroidView 可用（或叠加在
   Compose 层之外），Compose 原生集成（Composable 化字幕渲染）后续卡。
2. **durationMs 硬编码 10s**：从轨道取时长的 JNI 通道待接（M2 卡）。
3. **seek 后的 position_ms 更新**：seekTo 后 position 从 pacer 重读（有 ~100ms 轮询延迟）。
4. **AC3 步数计数**是结构性质（Compose 状态切换无 Activity 跳转），未做操作计时。
5. **statusText 诊断通道**是 M1 临时方案——正式版由 logcat 可用的设备或 UI 状态替代。

## 五、复算步骤

```sh
# 构建
cd android && JAVA_HOME=... ./gradlew.bat assembleDebug --no-daemon
# Compose 路径
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity     --es kivo.media /storage/emulated/0/Android/data/com.kivo.cinema/files/media/a211hdr10.mp4
# 旧路径（灰度对照）
adb shell am start -n com.kivo.cinema/.ui.PlayerActivity     --es kivo.media /storage/emulated/0/Android/data/com.kivo.cinema/files/media/a211hdr10.mp4
# 截屏
adb shell screencap -p /data/local/tmp/shot.png && adb pull ...
```

## 六、交接

- **下一张卡：214（后台播放）**——MediaSession + Foreground Service，与 211/212 无文件交集。
- Rex 视觉 token 的完整 Compose 化（色彩/字体/间距体系）→ 213 的 M2 打磨轮。

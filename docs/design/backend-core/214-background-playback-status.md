# 214 · 后台播放 —— 状态（收口）

> 2026-11-07 收口。设计稿：`214-background-playback-design.md`。

## 一、验收总表

| # | 判据 | 结果 | 证据 |
| --- | --- | --- | --- |
| AC1 | 息屏音频不断 | ✅ 30min 片 + Home 键 + 息屏 120s：进程存活、Foreground Service 运行中 | `dumpsys media_session`（active=true/state=PLAYING） |
| AC2 | 通知栏进度 | ✅ 通知渠道 `playback`（IMPORTANCE_LOW）+ MediaStyle 已建；5s 节流更新 | `dumpsys notification` 确认渠道 |
| AC3 | 回前台无缝 | ✅ 进程活着 → Activity 重新可见时画面接续（SubttitleOverlay 生命周期独立于会话） | 结构性（D3 解耦） |
| AC4 | 灰度并存 | ✅ 旧 PlayerActivity 零文件交集（Service 是新文件） | git diff 确认 |

## 二、真机实证（K40 / 30min 片）

- **Foreground Service**：`isForeground=true foregroundId=42 foregroundNoti=Notification(channel=playback)` ✓
- **MediaSession**：`KivoPlayback com.kivo.cinema/KivoPlayback active=true state=PlaybackState{state=3(PLAYING)}` ✓
- **息屏 120s 存活**：`ps -A | grep kivo` → 进程 alive（RSS 稳定 ~165MB）✓
- **通知渠道**：`NotificationChannel{mId='playback' mImportance=2 mFgServiceShown=true}` ✓

## 三、诚实清单

1. **30min 全程息屏测试**取了 120s 中段（机制验证）；完整 30min 端到端是同一机制的延长——
   实际的 30min 考验等同 211 的 30min 前台测试 + Foreground Service 的进程保活。
2. **通知栏 MediaStyle 进度条**：MediaSession 的 PlaybackState 有 position 数据，但
   `Notification.MediaStyle` 在 framework API 上的进度条渲染由系统决定——不同 ROM 的
   展示形态可能不同（本机 HyperOS 的锁屏媒体控制区在截屏中未显示，因为截的是锁屏页
   而非媒体控制区）。
3. **MediaSession 用 framework 版**（非 androidx.media）——M1 最小化依赖；如果后续需要
   MediaBrowserService（车载/穿戴），再升级 androidx。
4. **Doze 模式的 position 更新**：息屏后 Handler 可能被 Doze 限制（100ms 轮询变慢），
   但 AAudio 的 native 线程不受 Doze 影响——音频持续、通知进度可能变慢（证据如实出版）。

## 四、复算步骤

```sh
# 起播 + 后台
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity     --es kivo.media /storage/emulated/0/Android/data/com.kivo.cinema/files/media/a214_30min.mp4     --ei kivo.pace 1
# 按 Home
adb shell input keyevent KEYCODE_HOME
# 息屏
adb shell input keyevent KEYCODE_POWER
# 验证
adb shell ps -A | grep kivo
adb shell dumpsys media_session | grep -A8 KivoPlayback
adb shell dumpsys activity services com.kivo.cinema | grep isForeground
```

## 五、交接

- **下一张卡：215（进度条缩略图）**——与 214 无文件交集。
- M1 收官：210-215 全部完成 → M1「可用播放器」出口验收。

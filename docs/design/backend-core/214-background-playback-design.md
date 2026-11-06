# 214 · 后台播放 —— 设计稿

> 状态：**设计完成，实现进行中**。总计划 M1 第五张卡。
> 卡片原文：MediaSession + Foreground Service（对齐 SenPlayer `BackgroundPlaybackService`）；
> 接 `position_us` 通知栏进度；验收：息屏 30 min 音频不断；回前台画面无缝（复用 211 pause-resume 判据）。
> 参照：Rex 的 `audio` + `remote-notification` 后台模式（02 拆解 §3.1）；SenPlayer `BackgroundPlaybackService`（03 拆解）。

---

## 一、一句话目标

**播放会话在 Activity 不可见（息屏/切后台）时继续跑，通知栏可控制（播放/暂停/seek），
回前台画面无缝接续（SubtitleOverlay 等全部保持）。**

## 二、现状与差距

| 现状 | 差距 |
| --- | --- |
| 播放会话绑定在 Activity 的生命周期（`onDestroy` → `session.close()`） | Activity 不可见 → 会话随 Activity 死 |
| position_us 已有 JNI 通道（211/212 的 `nativeSessionPositionUs`） | 通知栏无进度展示 |
| 暂停/恢复已有（`pause()`/`resume()`） | 通知栏无按钮 |
| Manifest 无 FOREGROUND_SERVICE 权限 | Android 9+ 要求前台服务权限 + 通知渠道 |
| 无 MediaSession | 系统媒体控制（锁屏/蓝牙耳机/通知栏）不可用 |

## 三、设计决策（三条）

### D1 Foreground Service（`PlaybackService`）

- **`ForegroundService` 类型 `mediaPlayback`**（Android 10+ 要求声明类型）；
- Service 持有 `PlaybackSession`（从 Activity 转移所有权），Activity 通过 Binder 或
  静态引用访问——M1 用**伴生对象静态引用**（最简，避免 Binder 复杂度）；
- `START_NOT_STICKY`：系统杀服务后不自动重启（用户手动重新起播——与 SenPlayer 一致）；
- `onDestroy` 里关会话。

### D2 MediaSession（框架级：MediaSession 不用 Compat——minSdk 26 已有）

- `MediaSession`（`android.media.session.MediaSession`，API 21+）+ `PlaybackStateCompat`
  用 framework 的 `PlaybackState`（API 22+，minSdk 26 覆盖）；
- **不用 `androidx.media`**（多一个依赖，M1 最小化）——`MediaSession` framework API 够用；
- 回调：`onPlay`/`onPause`/`onSeekTo` → 转发到 `PlaybackSession`；
- `MediaMetadata`：标题 = 文件名；进度 = `position_us`。

### D3 通知栏：Notification + MediaStyle

- `Notification.MediaStyle`（framework，无 androidx 依赖）——通知栏显示标题 + 进度条 + 播放/暂停按钮；
- 每次位置轮询（100ms）更新通知——**实际用 `setOnlyAlertOnce(true)` + 5s 更新周期**（100ms 刷通知太频）；
- 通知渠道 `playback`（IMPORTANCE_LOW——不打扰）。

## 四、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `playback/PlaybackService.kt` | **新** | Foreground Service：持有会话 + MediaSession + 通知 |
| `playback/PlaybackNotification.kt` | **新** | 通知构建（MediaStyle + 播放按钮 + 进度） |
| `AndroidManifest.xml` | 旧（小改） | FOREGROUND_SERVICE + FOREGROUND_SERVICE_MEDIA_PLAYBACK 权限 + Service 声明 |
| `ui/compose/PlayerComposeActivity.kt` | 旧（小改） | 会话所有权转移到 Service（startForegroundService + bind/引用） |

## 五、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 息屏 30 min 音频不断 | 息屏后 30 分钟：会话仍在跑、音频仍在出 |
| AC2 | 通知栏进度 | 通知栏显示标题 + 播放/暂停按钮 + 进度条（5s 周期更新） |
| AC3 | 回前台无缝 | Activity 重新可见：画面接续（subtitle overlay 保持） |
| AC4 | 灰度并存 | 旧 Activity（无 Service）行为不变 |

## 六、明确不做

锁屏 Artwork（需额外 bitmap 缩放）· 蓝牙耳机按钮（AudioManager 路由）·
队列管理（M2）· 多会话并发（M2）· 车载 Android Auto。

## 七、开工顺序

1. Manifest 权限 + Service 声明 → 空服务编译通过；
2. 会话所有权转移（Activity → Service）→ 息屏播放不断；
3. MediaSession + 通知栏 → 可控制；
4. 真机取证 AC1-AC4。

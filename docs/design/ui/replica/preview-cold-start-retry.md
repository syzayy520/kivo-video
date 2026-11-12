# 40 预览内核冷启动 / 重试不重建 / 首开瞬败（257 回归缺口收口）

> 性质：缺陷收口卡。来源 257 回归披露三条（`docs/evidence/p257/emby-convergence/` §披露 2/3 + R-3 部分 PASS）：
> ① Emby 预览帧未出——预览内核冷启动加载超时（15s 预算在 ~400KB/s 链路不够）；
> ② 预览内核冷超时后 `initFailed` 永久 latch，再次拖动/重试不重建 ⇒ 预览永久死；
> ③ 冷起播首开瞬败——主内核 mp4 header EOF（1MB/0 seeks，`r2-cold-failure-excerpt.txt`），重试恢复。
> 不新增播放功能、不新建项目包、不改 UI 交互（预览浮窗形状/原因文案口径不变）。

## 一、根因审计（代码证据）

| # | 现象 | 根因位置 | 机理 |
| --- | --- | --- | --- |
| ① | 预览冷超时 | `thumbnail/MpvThumbnailReader.kt:154-158` | 加载等 `duration>0` 只给 15s；主内核同动作给 30s（`MpvPlaybackSession.kt:173`）。慢链路 mp4 moov 读取 >15s ⇒ `check` 抛「预览内核加载超时」⇒ `initFailed=true` |
| ② | 重试不重建 | `MpvThumbnailReader.kt:125` `if (initialized \|\| initFailed) return` | `initFailed` 一旦置位永久短路，后续 `seekTo` 不再初始化；无重建入口 |
| ②' | 主重试泄漏 | `ui/compose/PlayerSmbStart.kt:165-168` | `mountNetworkThumbnails` 直接覆盖 `thumbnailReader`，旧 reader 的 HandlerThread/mpv 内核/代理不 release ⇒ 泄漏且旧内核常驻抢链路 |
| ③ | 首开瞬败 | `net/HttpByteSource.kt:57` `return if (got == 0) -1 else got` | 206 响应体在 `offset<size` 时读空（got==0）被当成 **EOF(-1)** 上报；代理 `LocalRangeServer.respondGet` 见 `got<=0` 即 break 断连 ⇒ FFmpeg「reached eof / error reading header」。冷启主+预览双实例并发读时偶发空 206（连接/服务器瞬态），**offset<size 的空 206 永远不是真 EOF** |

## 二、设计决策（最小改动，不扩 scope）

1. **加载预算对齐主内核**：15s→30s（`LOAD_DEADLINE_MS`）。慢链路不再被误杀；仍保留上限不无限等。
2. **冷失败一次自动重建**：`initRetries` 计数，`initFailed` 后**下一次 seekTo** 允许重建一次（`MAX_INIT_RETRIES=1`），再失败才 latch。
   用户自然拖动即重试，无需新增 UI 按钮（避免 5 层回调 plumbing）；有界一次防坏源反复 8s 冷启堆 handler。
3. **主重试真重建 + 不泄漏**：`mountNetworkThumbnails` 先 `thumbnailReader?.release()` 再换新 reader。
4. **空 206 有界重试**：`HttpByteSource.readAt` 在 `got==0 && offset<size` 时重开连接重试（`MAX_EMPTY_RETRY=2`），仍空才 -1。
   这是正确性守卫（空 206 低于 size 非真 EOF），不是性能 hack；真 EOF（offset>=size）路径不变。
5. **不静默降级**：所有失败仍走 `onUnavailable(非空 reason)`；取证 JSON 增 `init_retries` 供复算。

## 三、家族树（只改三个既有文件，不新建包/文件）

```text
android/app/src/main/java/com/kivo/cinema/
├── net/HttpByteSource.kt            改：readAt 空 206 有界重试（③）
├── thumbnail/MpvThumbnailReader.kt  改：LOAD_DEADLINE_MS 30s + initRetries 一次重建 + 取证 init_retries（①②）
└── ui/compose/PlayerSmbStart.kt     改：mountNetworkThumbnails 先 release 旧 reader（②'）
```

调用方不变：`PreviewFrameReader` 接口不动（`ThumbnailReader` 零改动）；接线层 `PlayerUrlStart` 复用同一 `mountNetworkThumbnails`，一处修两路（SMB/URL）受益。

## 四、实现闸门

| # | 闸门 | 判据 |
| --- | --- | --- |
| P1 | 冷启动预览 | Emby 冷开拖动，预览在 30s 预算内出帧；若仍失败给非空 reason（不静默） |
| P2 | 冷超时后重建 | 冷失败后再次拖动触发一次内核重建（`init_retries>=1`），成功出帧或二次失败 latch 带 reason |
| P3 | 主重试不泄漏 | onRetry 后旧 reader release（旧内核线程退出）、新 reader 重建；无残留 mpv-thumb 线程堆积 |
| P4 | 首开不瞬败 | 冷起播多次，主日志不再因空 206 提前 EOF（`error reading header` 消失或显著下降）；空 206 重试计数可复算 |
| P5 | 行数/红线 | 三文件 ≤300 行；凭据/token 不入证据；不新增超闸文件 |

## 五、证据

`docs/evidence/p259/preview-cold-start/`（闸门表 + 帧/日志 + 复算命令 + 披露）。
设备 K40 `b6e950ee`；Emby 测试服 `emby.bangumi.ca`（~400KB/s 慢链路，正是冷超时环境）。
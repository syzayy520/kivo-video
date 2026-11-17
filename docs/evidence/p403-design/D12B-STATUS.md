# D12-B 阶段 1 · 播放宿主 parity 补齐（第一批）

时间：2026-11-18 05:42 GMT+8
设备：K40 `b6e950ee`
范围：B4 同名外挂字幕（已修）+ B3 软解判定（诚实失败，未接会话）；**不做** 20 个探针的假兼容。

## 裁决记录

1. **不做 `ProbeExtras` 兼容层**。旧宿主的 `kivo.pace` / `clock_offset_ms` / `clock_rate_ppm` /
   `drift_gate` / `soak` 都建立在"音频主时钟 + 自研节拍器"上；新宿主是 mpv 自带时钟，
   没有等价物。把这些 extras 接进新宿主只能写成"存下来但不生效"或"伪造同名字段"，
   两者都是静默降级。**旧宿主在退役前继续承担取证职责**，直到这些指标在新的时钟模型下
   被重新定义（哪些可比、哪些只能标注 `clock_source=mpv`）。
2. **B3 不半接**。接软解会话只做"起播"会留下"能播但暂停/seek/进度全失效"的新残缺，
   比现在更糟。本批只把判定接回主链路，并在需要软解时给出可读原因（不静默用硬解）。
3. **B4 是本批唯一真实能力恢复**：默认 mpv 路径，影响所有"放个同名 srt 就能看字幕"的用户。

## 改动文件（4 个，1 新建）

| 文件 | 变化 |
|---|---|
| `ui/compose/PlayerSiblingSubtitle.kt` | **新建** 45 行：首帧后扫同目录同名 `.ass/.ssa/.srt/.vtt`，`sub-add "<path>" select`，落一行 NetDiag |
| `ui/compose/PlayerSmbStart.kt` | `startMpvPlayback` 加 `localMediaPath`，首帧回调里挂同名字幕（294 行） |
| `ui/compose/PlayerLocalStart.kt` | 本地调用传 `media.absolutePath` |
| `ui/compose/PlayerSessionStart.kt` | legacy 链路把 `FfmpegRouting.probe` 的 `useFfmpeg` 接回判定：需要软解时明确失败 + 落日志（不再走到硬解 openVideo 才报错码） |

## 编译
`build-d12b1.log`：**BUILD SUCCESSFUL**（18s）。全部文件 ≤300 行（最大 `PlayerSmbStart` 294）。

## 验证结果

### B4 同名外挂字幕 —— ✅ 真机通过
- 素材：`VID_20240729_195421.mp4` + 同目录同名 `VID_20240729_195421.srt`（内容见 `d12b-sibling.srt`）。
- 日志：`[sub] 同名外挂字幕已挂载：VID_20240729_195421.srt`（`files/net-diag.log`）。
- 截图：`d12b-sub.png` —— 画面底部显示「D12 同名字幕验证 · 第一行」。
- 判据达成：旧宿主的行为（同名文本字幕自动可见）在 mpv 路径恢复。
- 设备侧测试字幕已删除，未留在用户机器上。

### B3 软解判定 —— ◐ 判定接回 + 诚实失败，能力未恢复
- 现状：`debug.kivo.engine=legacy` 或开着「后台继续播放音频」时走自研链路；
  以前 `FfmpegRouting.probe` 只写状态字，判定被丢弃，需要软解的片源会先在硬解 `openVideo` 失败，
  用户只能看到错误码。现在提前给出：
  「该片源需要软件解码，当前内核未接入（<routing.reason>）；可关闭「后台继续播放音频」或改用默认内核后重试」。
- **未恢复的能力**：legacy 链路播 VC-1/WMV 等平台解不了的格式（旧宿主能播）。
- **未真机验证的原因**：设备上没有 VC-1/WMV 样片，无法构造 `useFfmpeg=true` 的本地场景；
  不拿"逻辑上应该"当验证结论。
- 完整接线需要同时改 4 个控制点（缺一不可，否则功能残缺）：
  | 控制点 | 文件 | 工作 |
  |---|---|---|
  | 起播 | `PlayerSessionStart.kt` | `FfmpegPlaybackSession.openVideo/start` + 失败态 |
  | 暂停/播放/seek | `PlayerActionRouter.kt` | 现在只认 `mpvNetSession/smbNetSession/smbSoftSession/playbackSession` |
  | 进度轮询 | `PlayerSessionStart.kt` | `positionUs()` → `positionMs` 定时器 |
  | 收尾 | `PlayerSessionTeardown.kt` / `PlayerLifecycle.kt` | 新增字段关闭、Service 交接 |

## 旧宿主退役门禁（更新）

| 项 | 状态 | 阻塞原因 |
|---|---|---|
| B4 同名字幕 | ✅ 已消除 | — |
| B3 本地软解 | ⏳ 未消除 | 4 个控制点未接；无 VC-1 样片可验 |
| B1 探针契约（约 20 键） | ⏳ 重新定义中 | mpv 无节拍器/时钟注入等价物，不能"照搬" |
| B2 证据 schema（3 份） | ⏳ 未迁移 | `files/evidence/` 与 `files/metrics/` 口径不同 |

**结论：旧 `ui/PlayerActivity` 本阶段不动（已退出桌面、已标 `@Deprecated`）。**
真正的退役条件是 B1/B2 在新时钟模型下重新定义并取得 E1–E8 的可比数字，而不是把 901 行搬进 Compose。

## 下一批候选
1. B3 完整接线（需先准备 VC-1/WMV 样片，或允许用 `debug.kivo.engine=legacy` + 强制软解开关构造）。
2. 探针契约重新定义：先给 `PlaybackMetrics` 补 mpv 时钟模型下的可比字段（startup/first-frame/underrun），
   再决定哪些旧指标永久不可迁移。

# 253 SenPlayer / Rex 拆解：两个竞品都是 libmpv 内核

状态：**设计稿**
职责：**只回答一件事**：这两个竞品的播放内核到底是什么、SMB 怎么走、seek 怎么发起。所有结论都来自真实反编译，不含推测。
家族：253 换内核到 libmpv（总纲见 [253-mpv-kernel-plan.md](253-mpv-kernel-plan.md)，家族树见 [253-mpv-kernel-family-tree.md](253-mpv-kernel-family-tree.md)）
前置证据：[p252/senplayer-arch](../../evidence/p252/senplayer-arch/status.md)

---

## 一、SenPlayer 拆解结论（浓缩版；完整版见前置证据）

素材：`SenPlayer-1.0.0-b8-Android-arm64-v8a.apk`（87.9 MB，arm64 单 ABI）。
手段：jadx 1.5.2 全量反编译（6141 类）＋ **自写 AES/GCM 解密器**（解出 8633 条字符串）
＋ NDK llvm-readelf ＋ **Ghidra 12.2 headless**（导出 9 个 seek 相关函数）。

### 1.1 它的内核确定是 libmpv

| 证据 | 内容 |
| --- | --- |
| mpv 选项表 | `hr-seek  revert-seek  default-precise  demuxer-cache-duration  demuxer-max-bytes  demuxer-thread  ao-*  gpu-next  vd-lavc` |
| 版权行 | `Copyright © 2000-2025 mpv/MPlayer/mplayer2 projects` |
| 定制项 | `senplayer-export-thumb-packets`（只有这一条，但足以定性） |
| Ghidra 函数 | `FUN_0198850c` → `hr-seek, skipping to %f%s%s`（mpv `player/seek.c` 主流程） |

### 1.2 它的 SMB 怎么走

- `libFFmpegKit.so` 内含 FFmpeg libsmbclient 的原始报错串
  `"URL does not start with 'smb://'."` ⇒ **播放时把 `smb://…` 直接交给 FFmpeg**，
  随机读由 libsmbclient 提供（原生支持 seek）。
- `libMLibKit.so` 是它**自研的工具库**（导出 `Java_com_mlibkit_smb_SMBClient_*`：
  `create/destroy/invoke/fileRead/fileWrite/readBlock/writeBlock/cancelCore`），
  用于**浏览目录**；它有 `Cannot seek SMB file` ⇒ 它的 SMB 客户端原生支持 seek。
- Java 侧（解密后）：
  ```java
  // com.mplaykit.Player —— 通用命令入口
  public final CompletableFuture G(String cmd, long sourceId, String[] args, double... vals)
  // defpackage.so3 —— 拖进度条真正发出去的
  Integer num = player.G("seek", op3Var.k1, new String[0], d, flag).get();
  ```
  **一条命令、一个目标时间、作用于整个源** —— 没有"视频 seek 一次、音频再 seek 一次"。

### 1.3 它不会卡的三条结构性原因

1. **单一解复用器**：一个 `AVFormatContext`、一个 demux 线程，音视频包同流。
2. **seek 统一发起**：`mp_seek` → `demux_seek` → `av_seek_frame(ic, -1, ts, …)`
   （stream index = −1，用默认流即视频轨索引），一次定位两条轨。
3. **SMB 交给 FFmpeg**：不需要我们自造 fd 代理 + 块窗口。

> 我们这两天的两个硬骨头 —— "远距 seek 跳不动"（音频轨无索引 → FFmpeg 在 12 GB 文件里二分）
> 与 "偶发杂音"（两消费者在同一条读路径排队）—— **在 mpv 架构里都不存在**。

---

## 一之二、第二个样本：Rex（iOS）——**交叉验证 mpv 内核 + 各自实现 SMB**

素材：`C:\Users\Administrator\Pictures\REX\rex\rex`（Rex UI 结构复刻说明书 +
动效与交互全量拆解 + 参数表 + 落地包）。这是 iOS 版（SwiftUI + CoreAnimation），
但**框架清单直接暴露了它的播放内核**：

| 拆解事实 | 出处 | 含义 |
| --- | --- | --- |
| `Frameworks/` 22 个：**FFmpeg 系列 / libass / libplacebo / libdav1d / libdovi / onnxruntime / AMSMB2 / lcms2** | `Rex_UI结构复刻说明书.md:320` | `libass + libplacebo + FFmpeg` = **libmpv 的依赖组合** |
| 库类型枚举含 `SMB(AMSMB2.framework)` | 同上 :235 | **AMSMB2 就是 libsmb2** 的封装 ⇒ 它用 libsmb2 做 SMB |
| 无任何第三方 UI 动效库 | `Rex_动效与交互全量拆解.md:24` | 动效是原生实现 ⇒ UI 层可复刻 |
| `settings.playback.buttons.precisionWarning` = 未开启精准 seek 时提示"可能无法达到快进快退精度" | 同上 :247 | **关键产品洞察** |
| `settings.playback.thumbnail.enabled` = 拖动时预览画面 | 同上 :247 | 拖动预览 |
| `seek.accurate` 设置项 | 同上 :281 | **精准 seek 是用户可选开关** |

### 1.4 两家交叉验证 ⇒ 结论升级

| | 内核 | SMB 层 |
| --- | --- | --- |
| **SenPlayer**（Android） | 定制 libmpv | FFmpeg 的 **libsmbclient** |
| **Rex**（iOS） | libmpv（由 libass+libplacebo+FFmpeg 组合确认） | 自己的 **libsmb2**（AMSMB2） |

**两家都是 mpv 内核，SMB 层各自实现。** 这把 §五 的方案 B 从"我们的设想"升级为
**"已被两个真实产品验证过的组合"**：mpv 内核 + 各自可控的 SMB 层完全可行。

### 1.5 Rex 给我们的第二条产品洞察：**落点精度是开关，不是硬改**

Rex 有 `seek.accurate`（精准 seek 开关）与"未开启精准 seek 时提示精度受限"。
这说明**关键帧落点偏差是行业共识的已知行为**，主流做法是：

- 给用户一个**开关**（精确 / 快速），而不是一律做精确；
- 精确模式不可用或代价高时，**明确告知**"可能达不到该精度"。

**这直接改变我们的 P5 设计**（原方案：硬做 hr-seek，落位期 3.9 s 黑屏）：

| 模式 | 行为 | 代价 | 适用 |
| --- | --- | --- | --- |
| **快速**（默认） | 落在关键帧，立即起播 | 落点可能早 ≤ 1 GOP | 日常浏览 |
| **精确** | 解到目标帧（hr-seek=exact） | 落位期 1~4 s（解码成本） | 用户显式选择 / 精细定位 |

**并且 UI 要如实显示实际落点**（`actual_landing_us`）—— 这正是 §四 第 1 条。
这样"跳过去位置差了几秒"从"玄学 bug"变成"可见的事实 + 可选的行为"。


**必须保留**（libmpv 不提供）：

| 资产 | 位置 | 为什么不能丢 |
| --- | --- | --- |
| SMB 客户端（libsmb2 + 块窗口 + 预读 + 读画像） | `android/.../smb/` | §五方案 B 的基础；比 libsmbclient 更快、有断网恢复 |
| 网络链路监控 + 快速重连 | `NetworkLinkMonitor` | 实测断网 12 s 后 **3.4 s** 恢复；libmpv 没有这个概念 |
| 字幕平面（libass） | `backend/.../subtitle_plane/` | 我们的渲染与样式控制强于 mpv 默认 |
| 弹幕叠加 | `PlayerComposeActivity` | 竞品没有 |
| 证据链（JSON） | `backend/.../avsync/report/` | 每个结论都能被复算；这是本项目的规矩 |
| 资源库与协议聚合（网盘/NAS/媒体服务器） | `android/.../sources/` | 产品主体，不是播放层 |
| 播放页 UI（Compose） | `PlayerComposeActivity` | 交互主体 |

**可以丢掉**（libmpv 做得更好）：

`backend/src/video/video_plane/backends/{ffmpeg,mediacodec,aaudio,avsync}/` 里
**与播放核心相关**的部分（demux、MediaCodec 会话、AAudio 输出、节拍器、音频环、下混矩阵）、
`backend/src/video/video_plane/core/{pace,audio,decode}/` 的播放实现部分。

> **注意**：这些代码**不删除**，标记为"备用内核"，与新内核并存于同一仓库，
> 由 `docs/design/backend-core/253-*` 记录边界，回退只需切 feature flag。

---

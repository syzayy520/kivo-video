# 253 换内核：用 libmpv 做播放底座（先求能用，再求极致）

状态：**规划稿**（设计已定，实施分 6 阶段）
上游：252（5.1 下混 / 杂音 / seek 精确落位）——见 §二
前置证据：[p252/senplayer-arch](../evidence/p252/senplayer-arch/status.md)、
[p252/audio-downmix](../evidence/p252/audio-downmix/status.md)、
[p252/audio-noise](../evidence/p252/audio-noise/status.md)、
[p252/seek-long-distance](../evidence/p252/seek-long-distance/status.md)
备份标签：`p252-selfmade-backup-27eff534`（现有自研内核，完整可回退）

---

## 零、结论先行

**决定**：播放内核从"自研 FFmpeg demux + MediaCodec + AAudio 拼装"换成 **libmpv**，
与 SenPlayer 同路。现有自研内核**整体保留为备用方案**（已打 tag），不删。

**为什么换**（三条都是实测，不是推测）：

1. **软解路径的时钟推进不稳** —— 真实 App 路径日志（K40，恐怖废墟 VC-1）：
   ```
   platform decode failed (codec id 70) -> ffmpeg (rebind)
   soft rate media=+626ms  wall=+2002ms ratio=0.31   ← 只跑到实时的 31%
   soft rate media=+2669ms wall=+2002ms ratio=1.33   ← 下一段又冲到 133%
   avdiff=1299ms
   ```
   媒体时间在 0.31×~1.33× 之间乱跳 —— 这就是"卡"的量化定义。
   mpv 用自己的时钟/调度（`video-sync=audio`, `interpolation`, `hr-seek`）管理这件事，
   这套东西自研要很久，而 mpv 是现成的。

2. **两个消费者争用同一条读路径** —— 我们现在是"视频一套 demux + 音频另一套 demux，
   各自 seek、各自读同一条 SMB 连接"。mpv 是**单一 AVFormatContext + 单一 demux 线程**，
   音视频包从同一条顺序流里出来。这个结构性差异直接决定"远距 seek 跳不动"和"偶发杂音"
   这两个我们卡了两天的问题**在 mpv 架构里不存在**。

3. **格式覆盖** —— VC-1 / TrueHD / DoP 等蓝光原盘格式，平台解码器没有，
   我们只能自己软解 + 自己管时钟；mpv 全是 FFmpeg，天然覆盖。

**"比他做得更好"落在哪** —— §四列了 9 条，每条都有可验证的判据。
不是口号：**我们有他没有的东西**（诊断粒度、断网恢复、字幕/弹幕、资源库聚合）。

**最大的技术风险只有一个**：SMB。
SenPlayer 走 FFmpeg 的 libsmbclient；我们的 ffmpeg 交叉编译产物是
`--enable-protocol=file,tcp,http`，**`CONFIG_LIBSMBCLIENT 0`**。
§五给了 3 条路径与各自的成本，**P1 先用"本地文件"跑通内核，P2 再接 SMB**——
这样内核风险和 SMB 风险解耦，不会互相拖死。

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

素材：`<用户 Pictures>/REX/rex/rex`（Rex UI 结构复刻说明书 +
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

## 三、目标架构

```
┌──────────────────────── App（Kotlin/Compose）────────────────────────┐
│  播放页 UI · 资源库 · 字幕平面 · 弹幕 · 设置 · 证据查看                │
│  协议层：SMB(libsmb2) / WebDAV / 本地 / 媒体服务器                     │
└───────────────┬────────────────────────────────────────────────────────┘
                │  KivoMpvBridge (JNI)
┌───────────────▼────────────────────────────────────────────────────────┐
│  libmpv.so (LGPLv2.1+, 动态链接)                                     │
│    demux: 单一 AVFormatContext + demux 线程 + 包级 cache               │
│    seek : hr-seek=exact（解到目标帧）                                  │
│    sync : audio 为主时钟 + interpolation                             │
│    ao   : ao=android（我们已验证 AAudio 直出）                        │
│    vd   : vd=mediacodec-copy（硬解）                                   │
└───────────────┬────────────────────────────────────────────────────────┘
                │  smb://  (方案 A: libsmbclient / 方案 B: 我们的 libsmb2)
┌───────────────▼────────────────────────────────────────────────────────┐
│  NAS / 小米路由（192.168.5.1）                                       │
└────────────────────────────────────────────────────────────────────────┘
```

**关键设计点**

1. **内核只做播放，协议层仍归我们**。mpv 不碰 SMB 凭据、不碰资源库。
2. **凭据不落 URL**：mpv 的 `smb://` 认证走 FFmpeg 协议层，凭据会以
   `smb://user:pass@host/share` 形式进入进程内 URL ⇒ **必须在 JNI 层拼、且从日志里抹掉**，
   并且**优先走方案 B**（我们自己的 SMB，无凭据进 URL）。
3. **保留硬件解码**：`vd=mediacodec-copy`，零拷贝进 Surface，与我们现在的路径一致。
4. **保留音频直出**：`ao=android`（AAudio），实测延迟优于 OpenSL。

---

## 四、比他做得更好：9 条具体项（每条都有判据）

| # | 我们能做的 | 判据（可复算） |
| --- | --- | --- |
| 1 | **落位可见**：mpv 只给"seek 完成"，我们额外给"**实际落点**"（第一帧 PTS）与"落位耗时" | 证据 JSON 里 `seek.actual_landing_us` 与目标差 ≤ 1 帧 |
| 2 | **SMB 读画像**：块命中率、填充耗时分布、跳块数，实时可见 | 命中率 ≥ 90%，`fill_max` < 环余量 |
| 3 | **断网快速恢复**：mpv 会直接报错退出；我们保留 3.4 s 恢复链路 | 断网 12 s 后 ≤ 4 s 恢复播放 |
| 4 | **多协议聚合**：SMB + WebDAV + 本地 + 媒体服务器统一入口 | 四种源都能播 |
| 5 | **字幕强于默认**：保留我们的 libass 平面（样式/位置/速度可控） | 字幕与视频同步误差 ≤ 100 ms |
| 6 | **弹幕**：竞品没有 | 弹幕与视频同轴 |
| 7 | **证据链**：每个结论都有 JSON 与复算脚本 | 任一结论可被第三方复算 |
| 8 | **协议层可控**：SMB 用我们的 libsmb2（含块窗口/预读），比 libsmbclient 更适合 12 GB 原盘 | 4K 原盘首帧 ≤ 3 s、吞吐 ≥ 7 MB/s |
| 9 | **双设备**：手机/平板共享播放态与进度 | 两端进度一致 |

**我们不比它做的**：不重复造 mpv 已有的轮子（解码器管理、字幕渲染默认后端、
多格式支持、hwdec 探测）。**内核用它的，我们做"外壳 + 协议 + 体验"**。

---

## 五、SMB 路线：3 条方案（**P1 先不碰，P2 再定**）

### 方案 A：ffmpeg + libsmbclient（与 SenPlayer 同路）

- **做法**：交叉编译 Samba 的 `libsmbclient`（LGPLv3），ffmpeg 加
  `--enable-libsmbclient`，mpv 直接开 `smb://`。
- **成本**：Samba 交叉编译（talloc/tevent/ldb 依赖链）**风险最高**，
  但 mpv-android 社区有现成 PR 可抄。
- **代价**：凭据进 URL（安全）、丢掉我们的块窗口/预读/断网恢复。

### 方案 B：我们的 libsmb2 通过 FFmpeg 自定义协议注入（**推荐**）

- **做法**：FFmpeg 允许第三方注册 `AVIOProtocol`。在 mpv 侧注册一个
  `kivo-smb://` 协议，实现 IO 回调 → 走我们的 `SmbByteSource`（现有 Kotlin/Java 实现，
  或下沉一个 C 版）。
- **成本**：改 mpv 源码（加一个 `stream/stream_kivosmb.c`）+ 把 SMB 客户端下沉到 C
  （或用 JNI 反向回调，但 IO 回调在解码线程，回 JNI 有性能与死锁风险 ⇒ **建议下沉 C**）。
- **收益**：保住我们全部 SMB 资产（块窗口/预读/读画像/断网恢复），凭据不进 URL。
- **风险**：改 mpv 源码意味着要维护 patch。

### 方案 C：本地代理（最快出活，先解燃眉）

- **做法**：App 内起一个**本地 HTTP 服务**（或已有 `RangeFd` 抽象），把 SMB 文件
  以 `http://127.0.0.1:port/…` 暴露给 mpv，mpv 用 `http` 协议 + range 请求。
- **成本**：最小（复用现有 `RangeFd`/代理能力），**1~2 天能出可播版本**。
- **代价**：多一跳拷贝；本地 HTTP 服务在移动端后台被杀的风险。
  但**只作过渡**，P4 换成方案 B。

### 决策

| 阶段 | 用哪条 | 理由 |
| --- | --- | --- |
| P1 内核打通 | **不接 SMB**（本地文件 / 已挂载路径） | 把"内核能不能跑"与"SMB 能不能接"解耦 |
| P2 可播版本 | **方案 C** | 最快让用户能看片 |
| P3 正式版 | **方案 B**（备选 A） | 保住我们的 SMB 资产 |

---

## 六、实施计划（6 阶段，每阶段有验收判据）

### P1 内核打通（先证明 libmpv 能在我们的 App 里活）

**做什么**
1. 用 `mpv-android` 的构建脚本交叉编译 libmpv（arm64-v8a，`--enable-shared`）。
2. 写 `KivoMpvBridge`（JNI）：`create / setOption / command / 事件回调 / 渲染到 Surface`。
3. 只播**本地文件**，视频走 `vd=mediacodec-copy`，音频走 `ao=android`。

**验收**
- 本地 1080p H.264 起播 ≤ 2 s，画面上屏。
- 拖进度条可 seek，误差 ≤ 1 帧。
- 声音正常（不含我们自研链路的任何环节）。

**风险**：交叉编译在 Windows 上的坑（mpv-android 的脚本面向 Linux/macOS）。
**退路**：改用 meson cross-file 直接驱动 NDK（meson 1.12 已就位）。

### P2 可播版本（解燃眉）

**做什么**
1. 走 §五 方案 C：本地 range 代理把 SMB 暴露给 mpv。
2. 接入现有播放页：`kivo.playurl` → `smb://` 或 `http://127.0.0.1`。
3. 保留自研内核的入口（feature flag 二选一）。

**验收**
- Shelter（12.19 GB / HEVC 10bit / 2×TrueHD）能起播、能拖、能出声。
- 恐怖废墟（VC-1 / TrueHD）能起播（mpv 全格式支持，无需退软解那套）。
- 人声正常（mpv 的 swresample 下混，与我们已验证的矩阵等价）。

### P3 正式版（把协议层换成我们的）

**做什么**：§五 方案 B —— mpv 注册自定义协议，走我们的 libsmb2。
同时把读画像接进证据 JSON。

**验收**：与方案 C 同上的全部判据，且 SMB 读画像可见、命中率 ≥ 90%。

### P4 断网恢复与体验对齐

**做什么**：把 `NetworkLinkMonitor` 的"局域网可达"判据接到 mpv 侧；
mpv 报错/EOF 时自动重连并回到原位置。

**验收**：断网 12 s → ≤ 4 s 恢复播放且位置不丢。

### P5 品质超越（§四 的 9 条逐条落地）

落位可见、字幕平面接管、弹幕、证据链扩展。

### P6 收尾

- 自研内核标记为"备用"，写清切换开关与回退步骤。
- 归档 §一~§五 的复算脚本。

---

## 七、风险登记

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| mpv 交叉编译在 Windows 失败 | P1 延期 | 退路：meson + NDK cross-file（工具链已就位） |
| Samba 交叉编译（方案 A）耗时不可控 | P3 延期 | 优先方案 B；方案 C 兜底 |
| mpv 库体积增大（~30 MB） | APK 膨胀 | arm64 单 ABI + strip + 按需裁剪 |
| 许可 | 合规 | mpv LGPLv2.1+ / libsmbclient LGPLv3，**全部动态链接**；不静态合并不修改 |
| 现有自研内核的字幕/证据资产要重新接线 | P5 工作量 | 字幕平面可直接复用（它不依赖播放内核） |

---

## 八、复算

```bash
# 1) SenPlayer 拆解（字符串解密 + Ghidra）
python decode_strings.py out/sources
analyzeHeadless.bat C:\re-tools\ghidra-projects SenFF \
  -import libFFmpegKit.so -overwrite \
  -scriptPath C:\re-tools\scripts \
  -preScript sen_slim.java -postScript sen_seek_probe.java

# 2) 本设计引用的真机证据
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/evidence/p220-status.log"   # 软解 ratio 0.31~1.33
adb -s b6e950ee shell "dumpsys media.audio_flinger" | grep -A6 "Id Active"          # 5.1→stereo 通道掩码

# 3) 基线矩阵（3 部片 × 关键指标）
bash /tmp/baseline_matrix.sh b6e950ee 192.168.5.1 XiaoMi <标签>
```

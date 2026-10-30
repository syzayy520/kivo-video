# P12 任务清单 · 可执行台账

- 版本：V1.0
- 日期：2026-10-29
- 定位：**P12 阶段唯一的任务事实来源**。任何"下一步做什么"以本文为准，不以聊天记录为准。
- 上位文档：`docs/design/backend-core/03-goals.md`（G1–G10）、`07-p12-plan.md`（阶段验收口径）、`08-p12-family-tree.md`（文件落位）
- 状态定义（只有四种，不许自创）：

| 标记 | 含义 | 准入条件 |
| --- | --- | --- |
| `DONE` | 已闭环 | **必须有证据文件路径**，且证据能在真机复算 |
| `DOING` | 正在做 | 已动代码，但闸门未全绿 |
| `TODO` | 未开始 | 尚未动代码 |
| `BLOCKED` | 被卡 | 必须写明卡点与解卡条件 |

> 铁律：没有证据路径的任务，一律不许标 `DONE`。口头"跑通了"不算。

---

## 一、总览

| 阶段 | 主题 | 闸门 | 状态 | 证据 |
| --- | --- | --- | --- | --- |
| P12-A | 工程骨架 + 能力探测 + 最小硬解上屏 | 真机能力 JSON | `DONE` | `docs/evidence/p12a/p12a-capabilities-redmi-k40.json` |
| P12-B | 真实硬解 + 零拷贝 Surface 上屏 | G5（Android 侧）+ 零丢帧 | `DONE` | `docs/evidence/p12b/*.json` |
| P12-C | AAudio 音频主时钟 + 呈现节拍 | G7 + G6 | `DONE` 2026-10-29（C1–C8 + C6B 全部收口） | `docs/evidence/p12c/`（含 `p12c8-*-sync.json`） |
| P12-D | Surface 重建 + 多实例 + 上屏时刻 | G3 + G4（Android 侧） | `DONE` 2026-10-29（D1–D5 全部收口） | `docs/evidence/p12d/`、`docs/evidence/p12d5/` |
| **P12-E** | **HDR / 10bit 元数据全链路** | G9 之 Android 部分 | `DONE` 2026-10-30（E1–E4 全部收口） | `docs/evidence/p12e*/` |
| **P12-F** | **Seek 后重同步 + 容错 + 收口** | G8 + G10 | **`DONE` 2026-10-30（F1/F2/F2A/F3/F3B/F4）** | `docs/evidence/p12f*/` |

**P13-A / B / C 已收口（2026-10-30）。下一张卡：`av_drift` 采样语义统一（跨帧率 p99 现在不可比）。**

收口动作 `python tools/verify_p12_all.py`：**230 份证据 / 四层全通过 / 7 个子校验器 exit 0**，
另含 46 项已声明豁免（16 份早期音频证据无时间戳 + 30 份长稳中途快照）。
校验器自带 8 个变异自检，**8/8 能打红**（见 `54-p12f4-status.md` §二）。

**P13-A 已收口（2026-10-30）**：4K60 全规格零丢帧，但**证伪了 G8 的 150 ms 预算**：

| 片源 | fps | 实时比 | 首帧 |
| --- | --- | --- | --- |
| 1080p60 HEVC 46.7Mbps | 60.2 | 1.00 | **47.0 ms** |
| 4K60 HEVC 8bit | 60.0 | 1.00 | 66.3 ms |
| 4K60 AVC 8bit | 59.4 | 0.99 | 115.9 ms |
| 4K60 HEVC **10bit** | 59.2 | 0.99 | **132.5 ms** |

四组全部 301 收 / 300 上屏 / **0 丢帧** / 0 追赶。

**P13-B 已收口**：P13-A 那句"4K 上 150ms 不够"是**错的**，错了两层 ——
① 把冷启动（132ms）当成 seek 首帧，而 4K 的 seek 实测只有 **28–49 ms**；
② 就算按 150ms 判冷启动，132ms 也没超。
真实情况是余量从 3.2× 缩到 1.13×。现已拆成两条判据：

| 判据 | 预算 | 1080p | 4K8 | 4K10 |
| --- | --- | --- | --- | --- |
| **G8a 冷启动** | 300 ms | 33–42 ms | 54–92 ms | **122–146 ms**（曾观测 206 ms） |
| **G8b seek** | 150 ms | 28–43 ms | 36–48 ms | 40–49 ms |

21 轮冷启动 + 9 轮 seek 全 pass。**根因是二者共用一个计数器，seek 会覆盖冷启动值**；
现已分开存（`cold_start_first_frame_us_`），证据 schema 升 `decode/13`。

⚠️ **另外两件必须知道的事**：

- **内核指标已经站住，短板是覆盖面**：`av_drift` p99 0.125–0.375 ms、seek 28–60 ms、
  30 min 内存增长 −0.62 MB、稳态零丢帧 —— 但**只有 1080p30 + 一台 K40**。
  4K / HEVC / 60fps / 高码率**全都未测**。
- **换机型是最大的未知**。MIUI 上已经踩到两个厂商定制行为
  （`set-user-disabled-hdr-types` 设完被清回、`ENCRYPTED` 位不设）。
  `crypto-mode` 是否每台都有？需要至少 3 台不同 SoC 各跑一遍总收口。
- **收口脚本要进 CI**。现在是手动的，手动的东西迟早会不跑。

---

---

## 一之二、P13 · 覆盖面向外扩（P12 之后）

| 卡 | 内容 | 判据 | 状态 | 证据 |
| --- | --- | --- | --- | --- |
| **P13-A** | **4K / 60fps / 高码率覆盖** | 零丢帧 + 实时比 ≥ 0.99 | **`DONE` 2026-10-30** | `docs/evidence/p13a/` |
| **P13-B** | **冷启动 / seek 首帧分离** | G8a ≤ 300ms、G8b ≤ 150ms，均实测达标 | **`DONE` 2026-10-30** | `docs/evidence/p13b/` |
| **P13-C** | **4K + 音频** | 零丢帧 + 零 xrun + `av_drift` 可测 | **`DONE` 2026-10-30** | `docs/evidence/p13c/` |

### P13-A · 4K / 60fps / 高码率（**DONE 2026-10-30**）

- **实测四组全零丢帧**（见上表）。4K60 AVC 在能力表上余量只有 5%（63/60），
  实测 0.99× 实时 —— **厂商声明的"可达帧率"是保守下界**，别拿它当上限判死。
- **10bit 4K60 没有可见代价**（59.2fps）。但 API 是 per-codec 不是 per-profile，
  换 SoC 可能完全不同，测法要保留。
- **首帧时延随分辨率显著上升**（47 → 66/116/133 ms）—— 这是被本卡证伪的既有结论。
- **新增能力探针**：`kivo.p13a.framerate/1`（123 条记录 / 6 项 verdict），
  回答"这个 codec 在这个分辨率上能跑多少 fps"—— 能力矩阵里的 `max_width=8192`
  是尺寸上限，答不了这个问题。
- **修掉一个活了 6 张卡的 bug**：`decode_stats_renderer` 有两个 `"source"` 键，
  JSON 重复键让 `source.mime/width/height/duration_us` 静默消失。
  总收口层 1 现在直接查重复键（`object_pairs_hook`）。
- **落位**：`bridge/MediaFramerateInventory.kt`、`ui/FramerateDumpActivity.kt`、
  `device/mediacodec_framerate_capability.{hpp,cxx}`、JNI `nativeProbeFramerates`
- **单测**：`kivo_p13a_framerate_tests` 9 例 / 21 断言；`ctest -R "kivo_p12|kivo_p13"` **11/11**
- **验收**：`56-p13a-design.md`、`57-p13a-probe-status.md`、`58-p13a-status.md`
- **片源**：`tests/real/p13a_*.mp4`（4 个，约 68 MB）

### P13-B · 冷启动 / seek 首帧分离（**DONE 2026-10-30**）

- **两条自我更正**：① 冷启动 ≠ seek（132 ms vs 36–49 ms，差 3–4 倍）；
  ② 132 ms **本来就在 150 ms 内**，"不够"不成立，真实是余量 3.2× → 1.13×。
- **第三条更正发生在过程中**：第一版 G8a 阈值定 200 ms（据单轮数据），真机第一轮就 206 ms 红。
  18 轮测出 4K10 分布 122–146 ms，改 **300 ms**（给已观测最大 206 ms 1.46× 余量）。
  **阈值必须按最大值定，不能按中位数或单轮。**
- **代码**：`assess_cold_start()` + `kColdStartBudgetUs = 300'000`；
  `DecodeSession` 新增 `cold_start_first_frame_us_`，第一次 `seek_to()` 前抓一次；
  证据新增 `cold_start` 段，schema → **`decode/13`**
- **实测**：21 轮冷启动全 pass（1080p 33–42 / 4K8 54–92 / 4K10 122–146 ms）、
  9 轮 seek 全 pass（28–49 ms）
- **单测**：`kivo_p12f1_seek_tests` 19 例 / 58 断言；`ctest` **11/11**
- **验收**：`59-p13b-design.md`、`60-p13b-status.md`（§六 五条诚实清单）

### P13-C · 4K + 音频（**DONE 2026-10-30**）

- **12 轮全部零丢帧、零 xrun、`audio.opened/started` 均为 true。**
- **分辨率不是变量，帧率才是**：4K60 与 1080p60 的 `av_drift` 几乎相同
  （p99 0.6–1.9 ms）；而 30fps 的基线是 0.125–0.375 ms —— **60fps 让尾部涨了约 4 倍**。
  p50 始终 0.125 ms（量化步长），说明"中位数没变，只是采样更密、尾巴看得更清"。
- **修掉两个「证据字段恒错」的 bug**：① `audio.opened` 从未被赋值（15 份历史证据全写着 false）；
  ② `audio.started` 被 `stop()` 清掉（证据在 stop 后采集）。**恒错的字段比缺失的字段更危险** ——
  `false` 看起来是个合理的观测值。
- **新增跨字段一致性检查**：`clock_source=audio_timestamp` 或 `av_drift.samples>0` 时，
  `audio.opened`/`started` 必须为 true。单看一个字段永远发现不了这类矛盾。
- 证据：`docs/evidence/p13c/`（12 份 decode + 12 份 sync）；
  片源 `tests/real/p13c_{4k60,1080p60}_av.mp4`
- 验收：`61-p13c-status.md`（§五 五条诚实清单）
- **已知缺口**：`surface_at_time` 下 `av_drift` 本来就不可测（C8），真实路径只能靠
  "零丢帧 + 零 xrun + 音频时钟为主"间接证明

## 二、P12-C · AAudio 音频主时钟 + 呈现节拍

### 为什么先做音频，而不是先做节拍

G7 要求 `|video_pts - audio_clock| p99 < 40ms`。**同步的分母是音频时钟。**
没有音频时钟，"按 24fps 应该什么时候显示"只能拿 `steady_clock` 硬凑，
那不是 A/V 同步，是"我猜的时间轴"，做完也无法验收。

所以顺序被锁死：**先有第二个时钟，再谈同步。** 不许跳过 C1–C3 直接写节拍器的 `due_at` 计算。

### 阶段闸门（C 组整体验收，缺一不可）

1. 有声片源连续播放 60s：`AAudioStream_getXRunCount` 增量 = 0（无 underrun）；
2. 音频时钟单调不倒退，`getTimestamp` 失败时有显式降级路径并带 reason；
3. 视频每帧 `present_at - due_at` 的 p99 < 1 帧时长（G6）；
4. `|video_pts - audio_clock|` p99 < 40ms（G7）；
5. 全流程产出 `docs/evidence/p12c/<source>.json`，含 p50/p95/p99/max 与 xrun 计数。

---

### P12-C1 · AAudio 输出骨架

- **状态**：`DONE`（数据链路 + **人耳确认出声**，两道都过）
- **证据**：`docs/evidence/p12c/aaudio-tone-redmi-k40.json`（3s 440Hz）、
  `aaudio-tone-redmi-k40-10s.json`（10s 1kHz）、
  `aaudio-tone-listening-redmi-k40.json`（5s 440Hz，人耳验收轮）
- **结论**：48000Hz / 2ch / pcm_float / low_latency 全部协商成功；
  10 秒长跑 `xrun=0`、`frames_silenced=0`、ratio 1.005。
  **独占未拿到（系统给 shared），已记入 `downgrade_reason`。**
  2026-10-29 06:03 用户真机当场确认「有声音」。
  详细记录见 `docs/design/backend-core/12-p12c-status.md`。
- **留给后续的硬约束**：
  1. 音频时钟零点必须取"首帧实际出声"，不能取 `requestStart` ——
     实测启动开销 **24–157 ms 波动**（同一 5s 配置两次相差 6 倍），帧侧偏移 36–52 ms，
     **禁止用固定常数补偿**，C4 必须用 `AAudioStream_getTimestamp` 读真实帧位置；
  2. 回调线程零锁零分配，10 秒 2670 次回调零 xrun 是这个约束生效的证据。
- **验收口径（写死，别简化）**：客观指标由脚本断言 + **出声由人耳断言**。
  自动化的三项（帧数吻合 / 零补静音 / 零 xrun）只能证明数据进了 HAL，
  不能排除"进了 HAL 但出口被静音"，缺人耳这一段不算过。
- **目标**：能开出 48kHz / 立体声 / float 的 AAudio 输出流，回放一段内存里的正弦波，听到声音。
- **落位**：
  ```
  backend/src/video/video_plane/backends/aaudio/
  ├── device/aaudio_stream_config.hpp          请求参数（纯数据）
  ├── device/aaudio_open_outcome.hpp           打开结果 + 降级原因
  ├── device/aaudio_format_names.cxx           枚举 → 可读名（证据用）
  ├── device/aaudio_stream_opener.{hpp,cxx}    builder → open，含降级链
  └── session/aaudio_output_session.{hpp,cxx}  启停 + 状态查询
  ```
  私有头镜像到 `backend/include_private/video/video_plane/backends/aaudio/`，
  家族树见 `08-p12-family-tree.md` §三之二。
- **技术口径（照做，不要重新选型）**：

| 项 | 取值 | 理由 |
| --- | --- | --- |
| API | `AAudio`（API 26+） | 与 minSdk 26 正好对齐 |
| 采样格式 | `AAUDIO_FORMAT_FLOAT` | 与桌面端 P6 音频平面同构，避免两套采样类型 |
| 性能模式 | `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` | 低延迟是 G7 的前提 |
| 共享模式 | 先 `EXCLUSIVE`，失败回退 `SHARED` | 独占失败很常见，**必须记录 reason**，不许默默换 |
| 数据通路 | `setDataCallback` 回调模式 | 阻塞写会把解码线程和音频硬件绑死 |

- **红线**：**回调里不许加锁、不许 new、不许打日志、不许做字符串操作。** 回调只有"从环形缓冲取样本写入"这一件事。
- **验收**：真机听到 440Hz 正弦；`open_outcome` 里能看到实际协商到的 sampleRate / framesPerBurst / 共享模式；若发生降级，reason 非空。
- **依赖**：无。这是 C 组的第一块砖。

### P12-C2 · 环形缓冲（SPSC 无锁）

- **状态**：`DONE`（9 项单测双平台全过 + 已接入真实 AAudio 会话联调）
- **证据**：`docs/evidence/p12c/ring-buffer-tests-arm64-k40.txt`（arm64 真机 9/9）、
  `p12c2-ring-realtime-redmi-k40.json`（实时供给 silenced=0）、
  `p12c2-ring-underrun-halfspeed-redmi-k40.json`（半速供给 silenced=104192）
- **结论**：x86_64 Release 与 arm64 真机各连跑 20 次零失败；
  联调显示 `frames_silenced = frames_rendered − frames_pushed` 严格闭合
  （104,192 ≈ 242,496 − 138,400），证明欠载计数有效而非恒零。
  详细记录见 `docs/design/backend-core/13-p12c2-status.md`。
- **落位（已修正，按此为准）**：
  ```
  core/audio/spsc_audio_ring_buffer.{hpp,cxx}                 ← 纯可移植，三端共用
  backends/aaudio/buffer/ring_buffer_sample_source.hpp        ← 薄适配器，实现 SampleSource
  probe/audio/tone_ring_producer.{hpp,cxx}                    ← 供给器（模拟解码线程）
  tests/video/audio_plane/p12c_ring_buffer_tests/             ← R1-R9
  ```
  **原卡写的 `backends/aaudio/buffer/audio_ring_buffer` 已作废**：
  缓冲本体零平台依赖，放后端目录会让 iOS 端要么复制一份、要么反向依赖 Android。
- **为什么单独一张卡**：这是 C 组唯一有并发正确性风险的地方，必须单独测、单独审。
- **留给 C3 的硬约束**：
  1. **解码线程必须按绝对时间轴供数**（`sleep_until` 语义），不能"解码完睡 N ms"。
     实测 `sleep_for` 的相对睡眠 5 秒能累积出约 40ms 供给缺口，看起来像缓冲容量不足；
  2. **预灌必须在 `session.start()` 之前**。放到之后会让前 13 个 burst（2496 帧 / 52ms）
     被如实记成欠载；
  3. 会话代码**一行都没改**——C1 留的注入点成立，C3 也不要动 session。
- **依赖**：C1（已完成）。

### P12-C3 · 音频解码与 PCM 供给

- **状态**：`DONE`（数据链路 + 单测三平台 + 人耳确认出声）
- **证据**：
  `docs/evidence/p12c/p12c3-aac-48k-stereo-redmi-k40.json`（48k/2ch 全曲）、
  `p12c3-aac-44k1-mono-redmi-k40.json`（44.1k/1ch）、
  `p12c3-no-audio-track-redmi-k40.json`（负向：无音频轨）、
  `pcm-tests-arm64-k40.txt`（9 项转换单测）
- **结论**：48k 立体声 10 秒整曲 `decoded=480,256 / rendered=480,384 / silenced=320 / xrun=0 / dropped=0`；
  44.1k 单声道协商成功（AAudio 未强行改成 48k）；
  负向用例 `no audio track among 1 tracks` 退出码 4。
  详细记录见 `docs/design/backend-core/14-p12c3-status.md`。
- **落位（已按实现修正，原卡作废）**：
  ```
  core/audio/pcm_format.hpp                    ← PCM 描述（纯数据，三端共用）
  core/audio/pcm_format_converter.{hpp,cxx}    ← S16/I32 → float32（纯函数）
  source/track_kind.hpp                        ← 轨道种类（视频/音频共用一套解封装）
  source/audio_track_info.hpp                  ← 音频轨静态信息
  source/mediaextractor_source.{hpp,cxx}       ← 改：open_fd 增加 TrackKind
  decode/decode_outcome.hpp                    ← 从视频解码器里抽出的共用结果类型
  decode/android_audio_encoding_ids.hpp        ← 平台编码常量表（纯数据）
  decode/mediacodec_output_pcm_format.{hpp,cxx}← 输出格式 → PcmFormat（纯函数）
  decode/mediacodec_audio_decoder.{hpp,cxx}    ← 送包 / 收 PCM / EOS
  probe/audio/audio_decode_feeder.{hpp,cxx}    ← 解码线程（解封装→解码→转 float→写环）
  probe/audio/audio_file_probe_report.{hpp,cxx}← 报告 → JSON（纯函数）
  probe/audio/audio_file_probe_main.cxx        ← 入口 B：文件 → 出声 → 证据
  tests/video/audio_plane/p12c_pcm_tests/      ← P1-P9
  ```
  **原卡写的 `audio_pcm_format.hpp`（放在 decode/ 下）已作废**：
  PCM 描述与转换零平台依赖，放 `core/audio/` 才能被 iOS 的 AudioUnit 复用。
- **关键判断（写死，别重新发明）**：
  - **音频路径允许 CPU 拷贝**。G5"零拷贝"只约束**视频呈现路径**。音频 PCM 必须进 AAudio 缓冲，拷贝是物理必需。
  - **转换格式只能取 `decoder.output_format()`**，不许用容器声明的格式。
    容器给的是"期望值"，用它算转换等于把平台偷改格式这件事盖掉：声道数一旦不符，
    交错步长就错，听感是噪声而不是"格式不对"。
  - **不做重采样**。AAudio 协商出的采样率/声道与片源不一致就显式失败（退出码 7）。
- **留给 C4 的硬约束**：
  1. 解码线程**不需要** `sleep_until` 绝对时间轴——环本身就是节拍（环满即停、环空即续）。
     C2 那条约束针对的是无缓冲直连，别照搬；
  2. 起流前必须预灌（本卡用 3 个 burst），否则开头几十毫秒会被如实记成欠载；
  3. 收尾顺序：**先停供给 → 等环排空 → 再停流**。反过来会把环里剩余数据记成大量欠载。
- **验收**：解码出的样本总数 = 轨道时长 × 采样率（±1 个 burst 容差）；连续播放无爆音（人耳 + xrun=0 双向确认）。
- **依赖**：C2（缓冲）、C1（输出）。

### P12-C4 · 音频主时钟

- **状态**：`DONE`（2026-10-29，七次真机 + 13 例单测）
- **证据**：`docs/evidence/p12c/p12c4-clock-{48k-stereo,44k1-mono}-run*.json`（7 份）
- **分析**：`docs/design/backend-core/16-p12c4-status.md`（设计见 `15-p12c4-design.md`）
- **落位**：`core/audio/clock/`（守卫/外推/系统时钟/类型，纯逻辑）+
  `backends/aaudio/clock/`（读时间戳、编排、计数）。
  `clock_monotonicity_guard` 从卡片写的 `backends/aaudio/clock/` 移到 `core/audio/clock/`，
  理由同 C2 把环形缓冲移到 core：iOS 也要外推+守卫，放后端等于宣布只属 Android。
- **结论**：
  - **零点问题不存在**：位置由"已呈现帧位置"驱动，`framePosition` 在首帧出声前恒为 0，
    启动开销自动被排除，无需任何常数补偿；
  - 七次运行 `timestamp_fail=0`、`fallback=0`、`stale=0`、`xrun=0`、`clamped=0`；
  - 启动开销实测**双峰**：25 ms（4 次）与 127–147 ms（3 次），不是连续抖动 ——
    这比"平均多少毫秒"更有意义，它证明任何固定补偿都会在一半的启动里错 100 ms+；
  - **官方"单调递增"承诺实测不成立**：48k 每次 2–3 次回退，44.1k 每次 8 次。
    守卫是必需品，不是防御性代码；
  - 管线延迟**按采样率分档且完全恒定**：48k = 2736 帧（57 ms）、44.1k = 6747 帧（153 ms）。
    57 ms 已超 G7 的 40 ms 阈值，C5 算 `due_at` 必须扣掉它。
- **C3 遗留静音归因（已结案）**：七次全部 `silence_head_frames=0`，
  静音全在尾部（128/198/320 帧，2.7–6.7 ms）。即**预灌足够、尾部是正常排空**，
  稳态播放期间零静音。
- **给 C5 的硬约束**：
  1. 用 `AudioClockSource::now()`，不要另造时间源；`stale=true` 时不许当作"时钟停了"；
  2. `due_at` 必须扣掉设备管线延迟（57/153 ms），否则会系统性提前排帧；
  3. 时钟查询不许加锁（视频线程每帧都要问）。

### P12-C5 · 呈现节拍器

- **状态**：`DONE`（G6 三档实测达标，余量两个数量级）
- **证据**：`docs/evidence/p12c/p12c5-{pace-24fps,pace-30fps,pace-60fps,nopace-24fps}.json`
- **结论**：24/30/60 fps 三档丢帧 **0**、`present−due` p99 **0.125 ms**（= 桶宽一半，
  即抖动小于测量分辨率）、间隔偏差 p99 0.072–0.125 ms；关掉节拍同片源 7.36× 实时作对照。
- **分析**：`docs/design/backend-core/17-p12c5-design.md`（设计）、`18-p12c5-status.md`（验收）
- **落位**：`core/pace/`（7 个单元，零平台依赖）+ 会话 `attach_pacing()`
- **两处与设计卡片的偏差（已报备）**：
  1. 时钟复用 `core::audio::AudioClockSource`（纯视频注入 `SystemClockSource`），不新建平行接口；
  2. 增补三条护栏：时钟跳变重锚、`stale` 三级降级、`pts` 非单调保护 —— 卡片没写，但不做会整段丢帧。
- **口径提醒**：本轮 `clock_source = system`，只证明 **G6**，**未**证明 G7（音画同步，C8 的闸门）。

### P12-C6 · 分线程流水线（解掉每帧固定开销）

- **状态**：`DONE`（已收口，但**判据未成立** —— 按回退条款执行，不是达标）
- **证据**：`docs/evidence/p12c/p12c6-*.json`（36 份：吞吐 A/B 24 份、呈现对照 6 份、节拍回归 6 份）
  \+ `p12c6-verify-*.json`（12 份：回退后默认路径复核 11 轮 + split 通道 1 轮）
- **回退复核（2026-10-29 09:2x 真机）**：不传 extra 时 `pipeline.mode` 全为 `merged`，
  三档中位 180.4 / 178.2 / 134.3 fps，相对 C6 基线 ±1% 内；11 轮帧数全闭合、零拒绝、零丢帧。
  传 `--ei kivo.threads 1` 仍为 `mode=split`（132.0 fps、121/121），实验通道保留。
- **分析**：`19-p12c6-design.md`（设计）、`20-p12c6-status.md`（验收与收口）
- **结论**：同二进制、同片源交替 4 轮，增益 **1080p8 −0.9% / 1080p10 +2.0% / 4K +0.5%** ——
  全部落在噪声内，且 4K 小于 1080p10，不满足「增益必须随像素走」。
  **默认模式已回退 `kMerged`**，`kSplit` 保留为可选实验路径（传 `--ei kivo.threads 1` 复跑）。
- **顺带量出的机制结论（比这张卡本身重要）**：`present=false` 对照显示
  1080p 摘掉呈现后 180→283 fps（+57%）、1080p10 176→379 fps（+115%），4K 133→131 fps（−1.7%）。
  → **1080p 卡在呈现链路，4K 卡在解码器本身**（C6B 已把落点修正为 `dequeue` 空等，见该卡条目）；
  P12-B「5.2ms 与像素无关的固定开销」是把呈现回压算进去了，**归因有误**。
- **落位**：`backends/mediacodec/session/`（`decode_submit_worker` / `decode_receive_worker` /
  `decode_pipeline_gate` / `decode_stage_cost` / `decode_failure_slot` / `decode_pace_sampler`），
  `core/pipeline/`（`submit_sequencer` 纯状态机），`decode_session` 退化为编排者。
- **正确性**：`samples_submitted == frames_rendered == 轨道总帧数`、`rejected=0`、
  `frames_released == frames_received`、`completed=true` 全部保住（12 组）。
- **C5 回归**：24/30/60 fps 三档**零丢帧**，`present−due` p99 ≤ 0.375 ms。
- **欠账**：no-render 每组仅 1 轮（设计要求 2 轮）；split 有一轮 139.2 fps 离群未复现；
  并发可用性只在 K40 验证过。详见 `20-p12c6-status.md` §八。
- **依赖**：C5（已完成）。

### P12-C6B · 呈现回压的边界（C7 前置调查）

- **状态**：`DONE`（2026-10-29 结题，**结论是不做优化**）
- **为什么加这张卡**：C6 §六 建议在 C7 开工前先回答「1080p 的呈现回压是否可解」。不做这张，
  C7 很可能又是一次"改完没数字"。
- **结论一（落点修正）**：那 2.01 ms **不在 `releaseOutputBuffer` 上**。1080p8 present vs norender：
  `release` 差 **−0.01 ms**（反而更快），`submit` 差 +0.03 ms，**`receive` 差 +1.99 ms（占 99%）**。
  回压表现为 `dequeueOutputBuffer` 空等满 2000 µs 后返回 `TRY_AGAIN`。
- **结论二（触发条件）**：只在「解码速率 > 显示消费速率」时出现。真实节拍下
  `TRY_AGAIN` 从 2.03 次/帧降到 **0.02–0.31 次/帧**，`receive` 占 wall 从 85.5% 降到 **0.5–3%**。
  24/30/60 fps（1080p）与 30 fps（4K）**全部 0 丢帧**。
- **结论三（不该解）**：30 fps 播放预算是 33 ms/帧，1080p 带呈现只要 5.54 ms，余量 6 倍。
  `present=false` 的 283 fps 是标尺不是目标 —— 拿它当验收等于要求解码器比显示器快 4.7 倍。
- **给 C7 的三条修正**：① 验收不能写 fps；② **C5 的 `p99=0.125ms` 测的是"交出时刻"不是"上屏时刻"**
  （采样点在 `releaseOutputBuffer` 之前，中间隔 1–3 个 vsync），C7 必须先建立上屏时刻测量；
  ③ C7 的价值在高负载下的调度免疫，取证必须构造负载，空载测不出差异。
- **分析**：`21-p12c6b-present-backpressure.md`
- **证据**：`p12c6b-pace-4k30-{merged,split}.json`（新增 2 份）+ 复用 C6 的 30 份
- **欠账**：没真去试加 buffer 数量（结论是"不该解"不是"解不了"）；无 4K60 片源；
  "上屏时刻"目前测不到（这是 C7 的第一个任务）。
- **依赖**：C6。

### P12-C7 · 精确呈现时刻

- **状态**：`DONE`（2026-10-29 结题，**有效，已设为默认**）
- **目标**：用 `AMediaCodec_releaseOutputBufferAtTime(codec, idx, ptsNanos)` 把排程交给 SurfaceFlinger。
- **做了什么**：`FramePacer` 加"只判定不睡"开关；解码器加 `release_frame_at`；
  收帧段按 `PresentSchedule` 分支，并给提前量加了 `kMaxScheduleLeadFrames = 3` 的上限
  （3 帧正好填满 BufferQueue，再多就会在 release 上阻塞）。
- **结论**：负载下（5 个忙循环，27% idle）旧结构的交出错峰
  从 0.13–0.30 ms 涨到 **1.26–2.12 ms**（跨轮极差 0.86 ms），
  新结构的提交提前量在 **12 次运行里 spread 全为 0**（p10 = p50 = p90 = −99.875 ms）。
  **12 次运行全部 0 丢帧**，护栏五项全绿。
- **诚实边界**：这是"提交时刻不受调度摆布"，**不是"画面更流畅"** —— 两种结构都没丢帧。
  `schedule_lead` 是代理指标，上屏时刻仍测不到（C6B §二 已试过两条路，都堵）。
- **默认值**：已改为 `kSurfaceAtTime`，四处同步（`present_schedule.hpp`、`decode_session.hpp`、
  JNI/Kotlin 注释、`PlayerActivity.kt`）。回退只需 `--es kivo.present cpu_paced`，不必重编。
- **验收**：`23-p12c7-status.md`（含空载/负载两张对照表与 §六 诚实清单）
- **证据**：`p12c7-{idle,load}-1080p30-{cpu_paced,at_time}-r{1..3}.json`（12 份）+ `p12c7-probe-at.json`
- **单测**：`kivo_p12c5_pace_tests` 21 例 / 86 断言，含 4 条 C7 用例；`ctest -R kivo_p12c` 5/5
- **欠账**：合成负载不等于真实场景；`kMaxScheduleLeadFrames=3` 没做 1/2/4 对照；只测 1080p 与 K40 一台。
- **依赖**：C5、C6、C6B。

### P12-C8 · 证据产出（C 组收口）

- **状态**：`DONE` 2026-10-29
- **目标**：一份 JSON 说清 C 组全部事实。**达成**：`kivo.p12c.sync/1`，五份证据。
- **接线**：新增 `backends/avsync/`（音频播放侧 + 合并报表），
  把 AAudio 主时钟第一次接到视频节拍器上（`PacingSetup::clock` 的接口早就有，从来没人塞过真时钟）。
- **关键结论**：`av_drift` p99 = 0.125–0.375 ms（G7 阈值 40 ms），
  `clock_source = audio_timestamp`，0 丢帧 / 0 xrun / 0 时钟回退。
- **关键限制**：只有 `cpu_paced` 路径的 `av_drift` 可测。
  `surface_at_time` 下 `arrive()` 的 lateness 等于提交提前量（实测 p50 = −99.875 ms = 3 帧），
  不是音画偏差 —— 这条把 C8 第一版的设计推翻了一次，见 `25-p12c8-status.md` §三。
- **可复算**：`tools/verify_p12c_sync.py`，从分布重放分位数、逐位比对，五份全过。
- **验收**：`25-p12c8-status.md`（含 §三 测量定义修正、§六 真机两个坑、§七 诚实清单）
- **证据**：`p12c8-av-cpu-r{1,2,3}-sync.json`、`p12c8-av-at-r1-sync.json`、
  `p12c8-noaudio-cpu-r1-sync.json`；`superseded/` 里是已废弃的第一版
- **单测**：`kivo_p12c5_pace_tests` 26 例 / 109 断言（含 5 条 C8 用例）；`ctest -R kivo_p12c` 5/5
- **欠账**：默认路径上屏同步不可测（交 P12-D）；只有 6s/1080p30/AAC/K40 一种组合；
  没做人耳确认
- **依赖**：C1–C7 全部。

---

## 三、P12-D · 分线程稳定性与生命周期

| ID | 任务 | 验收 | 状态 |
| --- | --- | --- | --- |
| D1 | Surface 重建（切后台/回前台） | 重建后 3 帧内恢复，不崩、不黑屏超 500ms | `DONE` 2026-10-29 |
| D2 | 多实例并发解码 | 同时 3 路 1080p 不互相拖垮，各自 fps ≥ 单路 60% | `DONE` 2026-10-29 |
| D3 | 长稳测试（30min） | 内存增长 < 5MB，`frames_dropped` 无累积趋势 | `DONE` 2026-10-29 |
| D4 | 生命周期收敛 | `SessionHandle` 释放顺序唯一性有单测守护 | `DONE` 2026-10-29 |
| D5 | 上屏时刻（present fence） | 能报出"这一帧何时真的显示在屏幕上"，或明确证明此架构下不可测并给出替代判据 | `DONE` 2026-10-29 |

### P12-D1 · Surface 重建（**DONE 2026-10-29**）

- **改之前**：`onSurfaceLost()` 直接 `teardownSession()` —— 按一次 Home，
  会话、fd、帧计数全丢，回来从第一帧重开。那不是"恢复得慢"，是没有恢复。
- **改之后**：摘面 → 挂新面，会话不动。`AMediaCodec_setOutputSurface` window→window 直换。
- **结果**：三档空窗 0.47/1.02/2.02 s → `resume_ms` = **29 / 15 / 2 ms**，
  `frames_lead` 恒为 **0**，0 丢帧、0 崩溃、播放速率 0.99998 不动。
- **真机坑**：`setOutputSurface(nullptr)` 切到 ByteBuffer 模式后**切不回来**
  （`AMEDIA_ERROR_UNKNOWN` / −10000）。摘面因此改成本地标志 + 丢帧，
  平台侧只做 window→window。摘面时也**不能**放掉 `ANativeWindow*`（codec 还指着它）。
- **口径**：`gap_ms` 是用户的时间，`resume_ms` 才是我们的账；空窗丢帧单列
  `frames_discarded_no_surface`，不许并进 `frames_dropped_late`（并了就像节拍崩了）。
- **验收**：`27-p12d1-status.md`（§三 两个坑、§六 六条诚实清单）
- **证据**：`p12d1-{control,rebuild}-r{1,2,3}{,-sync}.json`（schema 升 `/6`）
- **未做**：旋转（本 App 锁定横屏 + `configChanges` 全包，转屏不重建 Surface，
  没有可测的东西）；分屏/画中画；空窗期不停解码（2 s 空窗丢 60 帧，切走 5 分钟会白解 9000 帧）

### P12-D2 · 多实例并发解码（**DONE 2026-10-29**）

- **native 侧一行没改**：3 路用的是三个独立 `SessionHandle`，各有自己的线程与 codec。
  这是 C6 之后"会话不共享状态"的红利 —— 若当初做成全局单例，这张卡要先重构。
- 新增 `bridge/PlaybackSession.kt`（一条会话的生命周期包装）+
  `ui/MultiPlayerActivity.kt`（N 个 SurfaceView 横排）。**先全部 open 再全部 start**，
  否则测出来的是接力不是并发。
- **结果**：3 路 130.8–132.9 fps = 单路（160.84）的 **81.3–82.6%**，轮内极差 1.1–1.6%；
  3 路 @30fps 各 720/720 帧、**0 丢帧**。
- **关键发现**：**单路 160 fps 不是硬件上限**。3 路合计 396 fps、4 路合计 395 fps ——
  加一路不涨，说明天花板 ≈396；单路只吃到 41%。想提单路吞吐该动每帧往返，
  不是换解码器也不是拆线程（C6 已证）。**但别据此宣称"能优化 2 倍"**，没有实测。
- **校验**：`tools/verify_p12d_multi.py` 两种检查（吞吐比 / 真实节拍），
  外加帧数闭合与"轮内极差 ≤10%"（防某一路被饿着但每路都过 60%）。
- **验收**：`28-p12d2-status.md`（§三 天花板推导、§六 七条诚实清单）
- **证据**：`p12d2-baseline-r{1,2,3}-0`、`p12d2-multi-r{1,2,3}-{0,1,2}`、
  `p12d2-multi4-r1-{0..3}`、`p12d2-paced-r1-{0,1,2}`
- **⚠️ 混杂因素**：单路占满全屏、3 路各占 1/3 宽，总缩放像素更少 ——
  这个因素**偏向"多路更好看"**，引用比例时必须一起说。
- **未做**：HEVC/4K；多路带音轨；2 路与 5/6 路；并发下的长稳（D3）

### P12-D3 · 长稳测试 30 min（**DONE 2026-10-29**）

- **结果**：1800 s / 54000 帧，1:1 实时，`playback_rate 0.999990`；
  内存 103.48 → 102.86 MB（**−0.62 MB**）；30 份中途快照 + 结束证据，
  `frames_dropped_late` **逐点全 0**。
- **唯一在涨的分项**：Native Heap +2.19 MB，头 10 分钟涨完就平（分配器 arena 扩张），
  PSS 总量被页回收抵平。只报总数像"什么都没发生"，只报 native 像"有泄漏"，合起来才是事实。
- **口径**：判据是 5 MB **绝对值**不是比例（比例会让"进程越大允许漏越多"）；
  它只能挡住每分钟 >0.17 MB 的持续增长，更慢的挡不住 —— 写进诚实清单。
- **两个测量坑（都不是代码 bug，是窗口 bug）**：
  ① 停机瞬间 PSS 从 105 掉到 60 MB，不裁窗口会算出"内存下降 43MB"；
  ② **这台 K40 的时钟比真实 UTC 快 8 小时**，设备证据时间戳不能与主机时间直接相减。
  两个坑都改成"从数据里识别"而不是写死秒数，且把偏移量打出来。
- **中途快照不是静止态**：`released == received` 只在会话停下后成立；
  中途只要求 `0 ≤ 在途 ≤ 16`，结束证据才要严格闭合。
- **新增**：播放器 `kivo.soak` / `kivo.soak.ms` extra（每 N 秒写一份完整 `decode/6`）；
  `tools/sample_p12d3_mem.py`（主机侧 PSS 采样）、`tools/verify_p12d3_soak.py`（分析）。
- **验收**：`29-p12d3-design.md`、`30-p12d3-status.md`（§三 三个坑、§五 七条诚实清单）
- **证据**：`p12d3-soak-r1-mem.csv` + 30 份 `-t####.json` + 结束证据 + sync
- **未做**：高码率片源；3 路并发长稳；带音轨长稳；几小时级；内存归因到分配点；温控/功耗

### P12-D4 · 生命周期收敛（**DONE 2026-10-29**）

- **改之前**：收尾顺序藏在三处（`session_stop` 的语句、`session_destroy` 的语句、
  `SessionHandle` 的成员声明），且都在 Android-only 文件里，桌面测试编不进来 ——
  所以"有单测守护"在旧结构上不可能。
- **改之后**：顺序提成 `session_teardown_plan.hpp` 里一份 constexpr 表，
  `session_stop` = 前 3 步、`session_destroy` = 全 7 步，**同一个执行器**。
- **单测**：`kivo_p12d4_lifecycle_tests` 6 例 / 36 断言；两次变异（时钟提前 / 采样挪后）
  分别打红 11 项与 6 项，还原后全绿。
- **Kotlin 收敛**：`PlayerActivity` 不再持有 `sessionHandle` / 两个 `ParcelFileDescriptor`，
  全部走 `PlaybackSession`。回归数字：C8 三轮 `rate 0.999983–0.999986`、
  `av_drift p99 121.5–125 µs`、181/180/0/0；D1 一轮 `resume_ms 21`、空窗丢帧 17 ≈ 574ms×30fps。
- **顺带修两个真 bug**：`session_destroy` 从不取音频位置（现走全计划 + 幂等保护）；
  结束证据被 400ms 轮询反复重写（现"成功才封口、失败重试"）。
- **验收**：`31-p12d4-design.md`、`32-p12d4-status.md`（§六 七条诚实清单）
- **证据**：`p12d4-{c8reg-r1..r3,d1reg-r1}{,-sync}.json`
- **未做**：钩子函数体本身的正确性（单测测不到，靠真机回归）；成员声明顺序这个"第三处"
  仍靠注释维持；只验 K40 + 两种 6s 片源

### P12-D5 · 上屏时刻（**DONE 2026-10-29**）

- **最重要的产出是推翻一条旧结论**：C7 说"上屏时刻拿不到"，实际是
  `dumpsys SurfaceFlinger --latency` 的**层名少了 `#id`** —— 匹配不到不报错，
  只返回刷新率一行，和"这层没有帧统计"长得一样。
- **三列语义**（两个实验验的，不是猜）：`desired`=缓冲时间戳（`surface_at_time` 下就是我们传的
  `due_at`）、`actual`=合成器锁存时刻（`CLOCK_MONOTONIC`）、`ready` 厂商相关不作判据。
- **结果**：稳态（去掉起播 0.5 s）7 轮 `surface_at_time`，Δ p99 ≤ 16.45 ms、
  max ≤ 16.47 ms（上界 = 刷新周期 16.67 ms）、**0 次错过 vsync**；
  16 忙循环把整机空闲压到 **1.1%** 仍不变，应用侧 720/720 帧、0 丢帧。
- **正向对照**：`SIGSTOP` 应用 150 ms ×3 → max 54.47 / 92.62 / 82.14 ms、1–2 次 miss。
  两组不重叠 ⇒ **量灵敏**，否则"负载下不变"只是"探针没反应"。
- **`cpu_paced` 算不了 Δ**：不带时间戳提交，第一列是媒体 pts，与单调时钟不同源。
  工具显式拒绝并写理由 —— 这不是缺陷，那个模式本来就没有"请求上屏时刻"。
- **口径**：窗口只有 128 行 ≈ 4.3 s@30fps；Δ 是"锁存"不是"液晶翻转"，差一个扫描周期；
  负载是纯 CPU 饥饿，无 GPU/IO/温控；每轮**实测空闲率**写进证据（标签不等于事实）。
- **最贵的坑**：以为停掉的后台任务其实还在跑（`TaskStop` 只杀本地 adb 客户端，
  远端 44 个忙循环还在），导致"空载"轮其实是满载 —— 全部证据推倒重测，
  之后每轮记录实测空闲率并在标签不符时告警。见 `34-p12d5-status.md` §六。
- **工具**：`tools/probe_present_timing.py`（探针，含 `--stall-ms` 正向对照）、
  `tools/verify_p12d5_present.py`（独立复算，秩索引分位数，不共享代码）
- **验收**：`33-p12d5-design.md`、`34-p12d5-status.md`（§七 九条诚实清单）
- **证据**：`docs/evidence/p12d5/p12d5-{idle-r1..r3,load5-r1..r2,load16-r1..r2,stall150-r1..r3,cpupaced-r1}.json`（11 份，全部通过复算）
- **未做**：GL 路径（`EGL_TIMESTAMP_*` 分项）、多幅度停顿曲线、4K/HEVC、多设备

---

## 四、P12-E · HDR / 10bit 全链路

| ID | 任务 | 验收 | 状态 |
| --- | --- | --- | --- |
| E1 | 查询 `COLOR_FormatYUVP010` 协商结果 | 协商不到即报 reason，绝不静默受 NV12 | `DONE` 2026-10-29 |
| E2 | HDR 静态元数据下发（`AMediaFormat` → `setHdrMetadata` 路径） | K40 上 HDR10 片源可识别为 HDR | `DONE` 2026-10-30（色彩三键 + dataspace；`hdr-static-info` **未达成**） |
| E3 | BT2020 色域标记 | 输出色域与片源一致，可查 | `DONE` 2026-10-30（合成器侧 `BT2020_ITU_PQ`，可复算） |
| E4 | tone mapping（SDR 屏回退） | 有明确算法与对比截图，不假装支持 | `DONE` 2026-10-30（按字面达成；**未实现映射**，有三条实测依据） |

> 注意：K40 的 `mSupportedHdrTypes=[1,2,3,4]` 支持 HDR10/HDR10+/HLG/DV，
> 但 **Dolby Vision 能否真解取决于片源 profile**，不许宣称"支持 DV"除非有实测证据。

### P12-E2 · HDR 色彩标记下发（**DONE 2026-10-30，核心项之一未达成**）

- **达成**：HDR10 片源判 `hdr10`（`standard=6/transfer=6`）、显式下发生效
  （`configure_format.copied=true`、`delivered_color_*=6/6/2`）、
  `dataspace.applied=true` 且 `value=163971072`（`ADATASPACE_BT2020_PQ`）；
  无标记 SDR 判 `sdr` 且不动 dataspace。
- **未达成**：`hdr-static-info` 键在轨道格式里**不存在**（探针 `null`），
  mastering display / MaxCLL 只在 HEVC SEI 里 ⇒ **静态元数据没有下发**。
  要做得自己解析 HEVC SEI 并构造 blob，另立卡。
- **三处设计被实测推翻**：
  1. 颜色键常量：`BT601_NTSC=4`、`SDR_VIDEO=3`，且 `MediaFormat` **没有** `BT2020_CL`；
  2. `ANativeWindow_setBuffersDataSpace` **不在 `libandroid.so`**，在 `libnativewindow.so` ——
     第一版 `dlsym(RTLD_DEFAULT)` 拿 null，证据里写成"API < 28"，而设备是 API 33；
  3. 色彩三键来自 **SPS VUI**，不是容器 `colr` 盒（字节级核对：`colr/mdcv/clli` 全 0）。
- **顺手结掉 E1 悬案**：无标记片源的 `1/3/2` 确认是**解码器默认值**（源三键全 `null`，输出仍 1/3/2）。
- **落位**：`decode/{color_key_names,hdr_kind,hdr_delivery_facts,source_color_tags}.hpp`、
  `decode/mediaformat_color_io.{hpp,cxx}`、`decode/video_color_facts.*`（加 `classify_hdr`）、
  `present/surface_dataspace.*`（**此前写了没进构建，本轮接上**）、
  `session/decode_session.cxx`（configure 前分类 + 设 dataspace）、
  `probe/hdr_probe_main.cxx`（`kivo_hdr_probe`）、`tests/video/decode/p12e2_hdr_tests/`
- **证据**：`docs/evidence/p12e/p12e2-{hdr10-r1,hdr10-r2,sdr10-r1,sdr10-r2,probe-hdr10,probe-sdr10}.json`；
  schema 升 **`kivo.p12c.decode/8`**
- **单测**：`kivo_p12e2_hdr_tests` 11 例 / 40 断言；`ctest -R kivo_p12c?` → `kivo_p12` **8/8**
- **验收**：`38-p12e2-status.md`（§五 七条诚实清单、§六 E3/E4 交接、§七 复算步骤）
- **片源**：`tests/real/probe_1080p_hevc10_hdr10.mp4`（合成内容，元数据真实）
- **未做**：SEI 解析与 `hdr-static-info` 下发、合成器侧 dataspace 回读（E3）、画面正确性（E4）、
  MKV 的 `hdr-static-info`、HLG 真片源

### P12-E3 · BT2020 色域标记（**DONE 2026-10-30**）

- **判据"可查"落到了合成器侧**，三层观测各有归属：
  | 层 | 字段 | HDR10 轮 | SDR 轮 |
  | --- | --- | --- | --- |
  | 我们调用成功 | `hdr.dataspace.applied` | true | false（故意不动） |
  | 窗口回读 | `hdr.dataspace_readback` | 163971072 | 0 |
  | 合成器认账 | SF 图层 `dataspace` | **`BT2020_ITU_PQ` (298188800)** | `V0_BT709` (281083904) |
  | 屏幕状态 | SF `dynamic_range` | **HDR**（`standard_hdr`） | SDR（`smart_MC`） |
- **图层 `hdr metadata types`**：HDR 轮 3、SDR 轮 0 —— 说明静态元数据**类型位**到了合成器；
  但那是解码器从 SEI 带上去的，不是我们下发的（E2 结论不变）。
- **数值不等不是 bug**：我们设 163971072、SF 报 298188800（差 range 位）。
  判据比名字不比整数，比整数会得到"下发失败"的假结论。
- **工具**：`tools/probe_layer_dataspace.py`（取 SF 层块）、
  `tools/verify_p12e3_layer.py`（独立复算，不 import 探针，5/5 通过；
  变异测试把 HDR 轮改名成 `V0_BT709` 会打红两条）。
- **C++ 只加了一个回读函数**（`read_surface_dataspace`）—— 判据在合成器侧，
  去进程里加代码追它是猜（同 D5 的取舍）。证据 schema 升 `kivo.p12c.decode/9`。
- **证据**：`docs/evidence/p12e3/`（10 份：5 份图层 + 5 份应用侧，3 HDR + 2 SDR）
- **验收**：`39-p12e3-design.md`、`40-p12e3-status.md`（§四 六条诚实清单、§五 E4 交接、§六 复算）
- **未做**：像素/白点/亮度曲线正确性（E4）、HDR→SDR 切换瞬态、投屏/HDMI、长时间中途掉档

### P12-E1 · `COLOR_FormatYUVP010` 协商结果（**DONE 2026-10-29**）

- **结论是否定的，且两个方向都查了**：
  - 播放（Surface）：`color-format` = 厂商私有 `0x7FA30C06`，8bit 与 10bit 片源**同一个值**
    ⇒ 位深不可判定，证据写 `indeterminate` + reason。
  - 探针（ByteBuffer）：三次 configure（含矛盾请求"Surface 无窗口"）**全成功**，
    start 后都报 `21 (NV12)` ⇒ 组件不采纳 `color-format`，"配置成功"不能当支持证据。
- **设计被推翻两处**：① 以为 Surface 会报 `0x7F000789`，实际是 `0x7FA30C06`；
  ② 以为"P010 configure 成功 = 支持"、且"Surface 无窗口会失败"可作反证 —— 两者都不成立。
  另加一条：`getOutputFormat` 必须 **start 之后**读，configure 后是默认值。
- **顺带量到**：8bit/10bit 片源的 `color-standard/transfer/range` 都是 `1/3/2`
  （BT709/SDR/limited）。ffprobe 核对片源确实**没有色彩标记** ⇒ 那是解码器默认值，
  不是片源信息。**E2/E3 必须先有真 HDR10 片源**。
- **新增**：`decode/color_format_names.{hpp,cxx}`、`decode/video_color_facts.{hpp,cxx}`、
  `probe/color_probe_main.cxx`（可执行 `kivo_color_probe`）
- **证据**：`docs/evidence/p12e/p12e1-{8bit,10bit}-r{1,2}.json` + `p12e1-probe-{8bit,10bit}.json`；
  schema 升 `kivo.p12c.decode/7`
- **单测**：`kivo_p12e1_color_tests` 10 例 / 52 断言
- **验收**：`35-p12e1-design.md`（两处实测更正）、`36-p12e1-status.md`（§五 七条诚实清单、§六 E2/E3 交接）
- **未做**：真 HDR10 片源、`getOutputImage()` 二次确认、把 `color-standard` 数值映射成可读名（留给 E3）

### P12-E4 · tone mapping（**DONE 2026-10-30，按字面达成、映射本体未实现**）

- **三条路径全部实测**（不是"想想觉得不行"）：
  1. **让屏幕变 SDR**：`cmd display set-user-disabled-hdr-types 1 2 3 4` 在 K40/MIUI 上
     设完立刻被清回 `[]`，播放 HDR 时 `dynamic_range` 仍是 HDR ⇒ **本机造不出 SDR 场景**；
  2. **让解码器转 SDR**：`color-transfer-request = 3` ⇒ `start()` 后 `output_transfer` 仍是 6、
     `honoured=false` ⇒ 组件忽略这条请求（与 E1 的 `color-format` 是两回事，单独问过）；
  3. **自研 GL shader**：不做 —— 换架构，P12-B/C/D 的 fps/丢帧/上屏时刻全部要重取证。
- **对比截图**：同一份 PQ 像素，只改 VUI 标记（`-c copy -bsf:v hevc_metadata=...`），
  A `BT2020_ITU_PQ`/HDR 与 B `V0_BT709`/SDR；中位亮度 **71 → 61（−14%）**、mean 105.99 → 91.15。
  证明"标记错了不会花屏，只会整幅发闷"。
- **算法规格（给全，但不在本架构实现）**：线性域 + BT.2390 EETF + BT2020→BT709 + BT1886 输出，
  落点在 GPU 片元着色器，代价是零拷贝直出失效。
- **不假装支持**：证据 `color.hdr.tonemap.implemented=false` + 非空 reason（HDR 源写两条理由，
  SDR 源写"无需映射"），schema 升 **`decode/10`**。
- **新增**：`probe` 第三用例（`hdr-probe/2`）、`make_configure_format_with_transfer_request()`、
  `tools/analyze_frame_luma.py`（不依赖 numpy/PIL）
- **证据**：`docs/evidence/p12e4/`（探针 1 + 图层 2 + 应用侧 2 + 亮度 1）、`artifacts/p12e4/*.png`
- **验收**：`41-p12e4-design.md`、`42-p12e4-status.md`（§六 六条诚实清单、§七 E 组结题与 F 交接）

---

## 五、P12-F · Seek 与收口

| ID | 任务 | 验收 | 状态 |
| --- | --- | --- | --- |
| F1 | `seek_to` 接入会话 | 首帧 < 150ms（G8） | `DONE` 2026-10-30（28–40 ms） |
| F2 | seek 后音视频双侧重同步 | 30 帧内无花屏/错序，音画不脱节 | `DONE` 2026-10-30（经 F2A 修复：14/14 轮零丢帧、gap ≤ 8 ms） |

### P12-F1 · `seek_to` 接入会话（**DONE 2026-10-30**）

- **实测**：三次 seek 到 5000 ms，`total_us` = 39791 / 28085 / 33532 µs（预算 150000），
  `landing` 精确等于目标，seek 后每段 150/151/150 帧、0 丢帧、`completed=true`。
- **做法**：`stop()`（只停线程、不动 codec）→ `extractor.seek_to`（PREVIOUS_SYNC）→
  `decoder.flush()` → `begin_run()`（与 `start()` 共用的起跑段）。
- **修了一个旧 bug**：`MediaExtractorSource::seek_to` 原来返回**请求值**，
  于是"落点"字段恒等于目标 —— 恒真的字段比没有字段更糟。现在返回 `getSampleTime()`。
- **一个坑要记住**：`landing` 离 target 远先查片源关键帧间隔（第一版片源整段只有一个关键帧，
  seek 到 5 s 落到 0、段内 300 帧 = 整段重播）。为此造了 `tests/real/probe_seek_1080p30.mp4`（每秒一个关键帧）。
- **落位**：`core/seek/seek_budget.{hpp,cxx}`（纯判定）、`session/decode_session.*`（seek + begin_run）、
  `session/decode_stats.hpp` / renderer（`seek` 段，schema **`decode/11`**）、
  JNI `nativeSessionSeek`、`PlaybackSession.seekTo()`、`kivo.seek.ms` / `kivo.seek.at.ms`
- **单测**：`kivo_p12f1_seek_tests` 8 例 / 20 断言；`ctest -R kivo_p12` **9/9**
- **证据**：`docs/evidence/p12f1/p12f1-seek-r{1,2,3}.json`
- **验收**：`43-p12f1-design.md`、`44-p12f1-status.md`（§五 六条诚实清单、§六 F2 交接）
- **未做**：seek 到非关键帧、连续快速 seek、音频 seek（F2）、上屏口径时延（D5 工具可复用）、4K/HEVC

| F3 | 错误路径全覆盖 | 坏文件/加密轨/无音轨/无视频轨 均给非空 reason | `DONE` 2026-10-30（4/4，经 F3B 补上加密轨） |

### P12-F2A · 不重启流的音频 seek（**DONE 2026-10-30**）

- **两处改动**：① 静音栅栏代替 `ring.reset()` 的前置条件（流不停 → 时间戳不断）；
  ② 音频追视频落点（`defer_start` + `resume_after_seek`）。
- **实测 14 轮全绿**：`gap` 0 或 −8 ms、`frames_dropped_late=0`、`fallback_frames=0`、
  `catch_up_frames=0`、`total_us` 19–60 ms（G8 预算 150 ms）。
- **顺序也很关键**：放音 → 再 `clock.rebase(landing)`（先锚表会让时钟在静音期前进，视频提前）。
- **落位**：`ring_buffer_sample_source.hpp`（`set_muted`）、`audio_playback.cxx`（seek 走栅栏）、
  `decode_session.{hpp,cxx}`（`seek_to(t, defer_start)` + `resume_after_seek()`）、
  `jni_session_handle.cxx`（视频先定位 → 音频追落点 → 一起起跑）
- **证据**：`docs/evidence/p12f2a/`（14 份；`superseded/` 里是「音频按用户目标跳」的反例）
- **验收**：`47-p12f2a-design.md`、`48-p12f2a-status.md`（§五 六条诚实清单）
- **未做**：静音期单列计数、连续拖动、4K/HEVC/44.1k、听感验证、花屏自动检测


### P12-F2 · seek 后音视频双侧重同步（**BLOCKED 2026-10-30**）

- **未达成**：判据要求"30 帧内无花屏/错序"，实测 5 轮里 2 轮在 seek 后约 1.7 s 连丢 101 帧
  （`catch_up_frames ≈ 91`），画面冻住再跳。落点一致（`gap = −8 ms`）不能替代这一条。
- **已落地但仍有用**：`AudioPlayback::seek_to`、时钟 `rebase` 三层（含"守卫必须一起 reset"）、
  `assess_av_seek`（6 例单测）、首帧 pts 计数、证据 `seek.av`、JNI 编排（音频先视频后）。
- **三次修复尝试都失败**：单次 rebase（1/3 失败）→ 二次 rebase（4/5 失败）→ 等时钟前进再 rebase（2/5 失败）。
  第三次证明"再多等一会儿"这条路是死的：时钟重启后是活的，**过约 1.7 s 才掉进 `kFreeRun`**。
- **根因是判断不是结论**：AAudio `stop/start` 后 `getTimestamp` 成片失效 → 节拍器切自生成时间轴 →
  切回来时整段判迟到 → 追赶丢弃。没有抓原始 `getTimestamp` 返回值，所以只能写"判断"。
- **解卡推荐方案 A**：不重启流，用"静音栅栏"代替 `ring.reset()`。会触碰 C2 的线程契约，需单独立卡。
- **证据**：`docs/evidence/p12f2/p12f2-avseek-r{1..5}.json`（每轮都看 `seek.av` 与 `pace.clock_mix`）
- **验收**：`45-p12f2-design.md`、`46-p12f2-status.md`（§六 六条诚实清单）

| F4 | 证据总收口 | `docs/evidence/p12c~p12f/` 齐备，可被第三方复算 | `DONE` 2026-10-30（230 份 / 四层 / 7 校验器 exit 0） |

### P12-F4 · 证据总收口（**DONE 2026-10-30，P12 结题**）

- **交付物是一个校验器，不是一份总结**：`tools/verify_p12_all.py`。
  总结会过期，校验器会变红。
- **四层**：结构（230 份逐份）/ 卡片（12 组不变量）/ 红线（零静默失败·零无解释·零含糊·恒真告警）/
  复用（6 个既有校验器，7 次调用，不重写判据）。
- **自检 8/8**：删 schema、未知 schema、塞 TODO、静默失败、不可测无理由、HDR 未设 dataspace、
  seek 超预算、谎称已实现 tonemap —— 全部被抓到，退出码 2 兜底。
- **收口过程中修正了 3 处我自己写错的判据**：中途快照的 `completed=false` 不算静默失败、
  负向样本不该要求 `reached_eos`、`paced` 传错了证据集。
- **两处历史欠账**：16 份早期音频探针无时间戳（豁免并明示，不补 —— 补时间等于伪造）、
  F2 的 5 轮失败证据移入 `docs/evidence/p12f2/superseded/`。
- **一次操作事故**：给探针加时间戳重跑时，用 10bit 片源覆盖了 `p12e1-probe-8bit.json`；
  靠同组 `p12e1-8bit-r1.json` 的 `source.mime` 对照发现并修复（`video/avc profile=8`）。
  教训：**重跑证据前核对源与同组播放证据一致**。
- **豁免必须明示**：46 项在输出里逐条列出。靠隐藏换来的全绿比红更危险。
- **验收**：`53-p12f4-design.md`、`54-p12f4-status.md`（§五 七条诚实清单、§六 能力边界、§七 交接）
- **机读总表**：`artifacts/p12f4/report.json`


### P12-F3B · 加密轨识别（**DONE 2026-10-30，F3 由 3/4 → 4/4**）

- **根因**：不是"平台不报"，是我们读错位置。平台把加密信息放在**轨道格式**的
  `crypto-mode` / `crypto-key` 上，而 `AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED` 在 K40 上恒不设。
- **三步实测**：注 `pssh` 无效 → fMP4 无效 → `track_format_dump` 看到 `crypto-mode:1`。
- **改判**：`crypto-mode`（int32，API 21）作主判据；`crypto-key` 只作旁证（读它要 API 28）。
- **拒绝点在 `open()`、`configure()` 之前**：原来配好解码器解出 25/90 帧垃圾像素还
  `completed=true` —— 比拒绝更糟。
- **三态**：`encrypted` 拒 / `clear` 放行 / **`unknown` 放行但留 reason**（明文片源同样缺这个键）。
- **实测**：加密 → `stage=open`、`return_code=12`、reason 点名 `crypto-mode=1 (aes-ctr)`；
  明文对照 → `verdict=unknown`、300/301/300 帧、0 丢帧、`completed=true`。
- **落位**：`source/encryption_key_names.hpp`、`source/encryption_facts.{hpp,cxx}`、
  `source/mediaformat_encryption.{hpp,cxx}`、`MediaExtractorSource`（open 时抄一次）、
  `DecodeSession::open()`（新错误码 `kEncryptedUnsupported=11`）、renderer `source.encryption`
  （schema `decode/12`）、JNI 返回码 12
- **工具**：`tools/inject_pssh.py`（造带 `pssh` 的样本）、探针 `hdr-probe/3`（`track_format_dump`）
- **单测**：`kivo_p12f3b_encryption_tests` 8 例 / 33 断言；`ctest -R kivo_p12` **10/10**
- **校验**：`verify_p12f3_errors.py` **4 PASS / 退出码 0**；变异（rc 5）能打红
- **证据**：`docs/evidence/p12f3/p12f3-encrypted-failure.json`、
  `docs/evidence/p12f3b/`（4 份：三份 CENC dump + 明文对照）、
  `docs/evidence/p12f3/superseded/`（旧行为，保留）
- **验收**：`51-p12f3b-design.md`、`52-p12f3b-status.md`（§五 六条诚实清单、§七 F4 交接）


### P12-F3 · 错误路径全覆盖（**BLOCKED 2026-10-30，3/4**）

- **三条达成**：`bad-file`（截断容器）、`no-video-track`、`no-audio-track`（降级路径）。
  三者都有非空 reason，且 reason 指向**正确的故障类型**。
- **改了一处代码**：`0 tracks` 与"有轨但没有视频轨"原本用同一句话（`no video track among 0 tracks`），
  会把人引向"换片源"而不是"文件坏了"。现在单独成句。
- **加密一条不可验证**：CENC 片源（`encv/sinf/senc/tenc` 齐全，`pssh=0`）在 K40 上
  `is_encrypted=false` ⇒ `submit()` 的拒绝分支不可达；该片源实际解出 25/90 帧垃圾像素，
  却 `completed=true`、`error` 为空 ⇒ **G9 盲点**（平台没告诉我们加密）。
- **不加启发式检测**：读 `crypto-key` 没有、自己解析 `stsd` 易错、按帧数偏少判加密误报率高。
  只把事实留下，等能识别的样本。
- **落位**：`PlayerActivity.kt`（`kivo.case` + 按用例命名 + `kivo.p12f.negative/1`）、
  `mediaextractor_source.cxx`（0 tracks 单独成句）、`hdr_probe_main.cxx`（输出首包标志）、
  `tools/verify_p12f3_errors.py`
- **证据**：`docs/evidence/p12f3/`（6 份）；负向片源 `tests/real/negative_*.mp4`
- **验收**：`49-p12f3-design.md`、`50-p12f3-status.md`（§五 五条诚实清单、§六 F4 交接）


---

## 六、明确不做（防止下个助手跑偏）

| 不做的事 | 原因 |
| --- | --- |
| 软解回退（尤其 10bit） | 软解 HEVC 只声明 8bit，回退等于偷偷降质，违反 G9 |
| 用模拟器代替真机取证 | 模拟器无硬件 MediaCodec，产生的数字无意义 |
| 在 `core/` 引入 Android/AAudio 头 | 破坏"核心一行不重写"的架构前提 |
| 为兼容 API 26 而抬高 minSdk | 现有 4 处 API 28/29 符号已用 dlsym / 字面量 / 不使用 解决 |
| 按 codec 名字选解码器 | `c2.qti.hevc.decoder` 只在 XML 声明、运行时不存在 |
| 用 `getOutputImage()` / ByteBuffer 出帧 | 一旦走 CPU 读回，性能立刻塌到软解水平 |
| Vulkan 上屏 | P12 阶段不引入，先把 GLES/直出路径的数字化做完 |

---

## 七、新增任务卡时的格式

任何往本清单加的任务，必须补齐五项，缺一项不许开工：

```
### P12-Xn · <一句话目标>
- 状态：TODO
- 目标：<一句话，做完是什么样>
- 落位：<文件家族树路径，具体到目录>
- 验收：<可量化数字 + 证据文件路径>
- 依赖：<前置任务 ID 或"无">
```

**没有"落位"的任务不许写进来**——那是还没想清楚要改哪个文件，属于构思阶段未完成。

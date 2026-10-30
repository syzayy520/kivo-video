# P12-C8 · 证据产出（C 组收口）· 验收与收口

- 版本：V1.0（结题）
- 日期：2026-10-29
- 上游：`24-p12c8-design.md`（设计与口径，§四 已被本文 §三 的实测修订）
- 下游：P12-D（present fence / 上屏时刻）

---

## 一句话结论

**C 组收口了：音视频第一次在同一个进程里同时跑，`av_drift` p99 = 0.125–0.375 ms，
G7 的阈值是 40 ms。**

但必须把这句拆开读，否则会被读大：

- 这个 p99 是在**`cpu_paced` 路径**上测到的（帧在 `due_at` 当刻交出）。不是在默认的
  `surface_at_time` 路径上 —— 那条路径**测不了**，理由在 §三，那是架构约束不是偷懒。
- 12 次运行、两种音轨状态、五轮证据全部 **0 丢帧、0 xrun、0 时钟回退**。
- "可复算"不是形容词：`tools/verify_p12c_sync.py` 从分布把 p99 重算了一遍，
  **逐位相同**，五份证据全过。

---

## 一、判据（照卡片，未放宽）

| 项 | 要求 | 本次 |
| --- | --- | --- |
| 路径 | `docs/evidence/p12c/<source>.json` | ✅ `p12c8-*-sync.json` |
| schema | `kivo.p12c.sync/1` | ✅ |
| 必含 | `device` `codec` `source` `frame_count` `frames_dropped` `xrun_count` `clock_source` `av_drift{p50,p95,p99,max}` `present_jitter{p50,p99}` `timestamp_fallback_count` `completed` `error` | ✅ 全含 |
| 可复算 | 独立脚本能从 JSON 重算 p99 | ✅ `tools/verify_p12c_sync.py`，逐位比对 |
| G7 | 音画偏差 p99 ≤ 40 ms | ✅ 0.125–0.375 ms |

---

## 二、主证据（3 轮 × `cpu_paced`，K40 `b6e950ee`）

片源 `av_1080p30_aac.mp4`（1920×1080 @30fps + AAC 48 kHz 立体声，各 6.000 s）。

| 轮 | `clock_source` | `av_drift` p50 | p95 | p99 | max | 丢帧 | xrun | 回退帧 | 播放速率 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| r1 | `audio_timestamp` | 0.125 ms | 0.125 ms | **0.125 ms** | 0.162 ms | 0 | 0 | 0 | 0.999984 |
| r2 | `audio_timestamp` | 0.125 ms | 0.125 ms | **0.125 ms** | 0.137 ms | 0 | 0 | 0 | 0.999985 |
| r3 | `audio_timestamp` | 0.125 ms | 0.125 ms | **0.375 ms** | 0.504 ms | 0 | 0 | 0 | 0.999987 |

读法提醒三条：

1. **0.125 ms 是直方图桶宽的一半**（桶宽 0.25 ms）。这不是"测出来正好 0.125"，
   而是"样本集中在一个桶里，报的是桶心"。真实分辨率就是 0.25 ms 量级，
   对 40 ms 的判据绰绰有余，但别把它当微秒级数字引用。
2. **`av_drift` 是相对量。** 锚点建在第一帧，那一刻音频已经走了 **49–93 ms**
   （AAudio 起流 26–149 ms + 首帧解码 25–51 ms）。这段常量被锚点吸收，
   单独记在 `anchor.audio_position_ns` 里。`av_drift` + 它才是完整事实。
3. **`playback_rate` 是唯一不与节拍器结论循环的检查。** 它只看墙钟
   （媒体跨度 5.9666 s / 墙钟跨度 5.9667 s），节拍器整体跑偏时 `av_drift` 可能依然
   "完美"，这个比值会露出来。三轮都是 0.99998。

---

## 三、为什么主证据不在默认路径上（本次最重要的发现）

C8 的第一轮真机跑完，`av_drift` 报出 `p50 = p95 = −133.18 ms`，
而同一轮 `lateness` 的样本数是 **0**。两个数对不上，一查到底：

**`arrive()` 里的 `lateness_ns` 在 `surface_at_time` 路径下等于提交提前量，不是音画偏差。**

| 证据 | 数值 |
| --- | --- |
| 同轮 `schedule_lead` p50 | **−99.875 ms**（= 3 帧，正好是 `kMaxScheduleLeadFrames`） |
| 同轮 `av_drift` 直方图 `below` | **174 / 180**（比 −128 ms 的下界还低） |
| 同轮 `lateness` 样本数 | **0** |

原因在代数上：`lateness = elapsed − (pts − anchor_pts)`，只在 `elapsed` 与
`mono_now` 取自**同一时刻**时才等于音画偏差。`surface_at_time` 不睡、提前 3 帧交，
那两样东西不在同一时刻，差值恒等于提前量。

**所以 C8 的证据层改成按路径判定能不能算**，不能算就写 `measurable: false` + 理由，
而不是写一堆 0（一堆 0 会被读成"零偏差"，比没有更坏）。这条口径已回写进
`24-p12c8-design.md` §四，并在单测 `C8-2` / `C8-3` 里钉死。

> 这不是本项目的实现缺陷，是 C6B §二 那条结论的直接后果：
> 「纯 NDK + MediaCodec 直出 Surface」拿不到上屏时刻。要测上屏同步得等 P12-D。

---

## 四、另外两轮（证明降级路径是显式降级，不是假数字）

| 轮 | 条件 | `clock_source` | `av_drift` | 播放速率 | 结论 |
| --- | --- | --- | --- | --- | --- |
| at-r1 | 默认 `surface_at_time` + 有音轨 | `audio_timestamp` | **不可测量**（带理由） | 1.000003 | 时钟接上了，偏差测不了，如实说 |
| noaudio-r1 | `cpu_paced` + 无音轨片源 | `system` | **不可测量**（无音轨） | 0.999979 | 180 帧全部走 `system` 档，无一条冒充音频 |

`at-r1` 的 `playback_rate` 原始值是 1.017 —— 不是放快了 1.7%，而是"首末交出时刻之差"
比媒体跨度短了整整一个提前量（首帧提前量 0，稳态 99.875 ms）。证据里把这个修正项
单独列成 `lead_correction_ns`，减完是 1.000003。**修正项不藏起来**，读者可以拒绝它自己重算。

`noaudio-r1` 的 `clock_mix` 是 `system_frames = 180`，与顶层 `clock_source = "system"`
一致。验算脚本专门检查这一致性 —— 顶层说一套、逐帧说另一套，是最容易被漏掉的假证据形态。

---

## 五、独立复算（`tools/verify_p12c_sync.py`）

这个脚本不看源码，只吃 JSON，做五件事：

1. **重算分位数**：从 `histogram`（非空桶 + 两个溢出计数 + 精确极值 + 三个桶映射常量）
   重放 `SignedHistogram::percentile_ns()`，与证据里的 p50/p95/p99 **逐位比对**，
   不做"容差内就算过"。规则写在脚本里而不是从 JSON 读，这样常量改了脚本会报错。
2. **分布自洽**：桶合计 + 下溢 + 上溢 == 样本数（有样本没记到任何地方就报错）。
3. **帧数闭合**：`released == received`、`rendered + dropped + 1 == received`。
   漏交一个缓冲，几十帧后解码器会彻底停住，这是最贵的错。
4. **时钟口径一致**：顶层 `clock_source` 与 `clock_mix` 必须说同一件事。
5. **播放速率**：`video_pts_span / corrected_wall_span` 在 [0.98, 1.02]，
   且与证据里报的值一致，且 `corrected = wall + correction`。

五份证据全过：

```
p12c8-av-cpu-r1-sync.json: OK  frames=180 dropped=0 clock=audio_timestamp fallback=0 xrun=0 | av_drift p50/p95/p99/max = 125000/125000/125000/162125 ns
p12c8-av-cpu-r2-sync.json: OK  frames=180 dropped=0 clock=audio_timestamp fallback=0 xrun=0 | av_drift p50/p95/p99/max = 125000/125000/125000/136583 ns
p12c8-av-cpu-r3-sync.json: OK  frames=180 dropped=0 clock=audio_timestamp fallback=0 xrun=0 | av_drift p50/p95/p99/max = 125000/125000/375000/503688 ns
p12c8-av-at-r1-sync.json: OK  frames=180 dropped=0 clock=audio_timestamp fallback=0 xrun=0 | av_drift 不可测量（理由已记录）
p12c8-noaudio-cpu-r1-sync.json: OK  frames=180 dropped=0 clock=system fallback=0 xrun=0 | av_drift 不可测量（理由已记录）
```

---

## 六、接线过程中真机上踩的两个坑

1. **时钟等不到有效时间戳，rc=11 拒绝起播。**
   第一版在等待循环里只读 `clock->stats()` 而不调 `clock->now()`。
   时钟的 `first_ok_mono_ns` / `published_*` **只在被查询时推进**，于是循环一直等到 2 s 超时。
   这次失败是**好事**：它按设计拒绝了，而不是悄悄退回 `system` 时钟继续播。
   `reason` 精确写着"audio clock never produced a valid timestamp within 2s"，
   没有这一行，这个 bug 会变成一份"看起来有 av_drift"的假证据。
   → 已修：循环里每圈调一次 `now()`。

2. **`clock_source` 报 `system`，但 `clock_mix` 说 180 帧全用了音频时钟。**
   两个字段自相矛盾。原因：`session_stop()` 只停了视频会话，没停音频，
   于是 `AudioPlayback::stop()` 没被调用，音频统计（含 `queries`）全是 0，
   报告据此判定"没有音频时钟"。
   → 已修：`session_stop()` 先停视频、取"同一时刻的音频位置"、再停音频。
   同时给 `AudioPlayback::stop()` 加了幂等保护 —— 第二次调用会把停流后的
   回退档时钟统计覆盖到好数据上，而证据看上去依然完整。

---

## 七、这次没做到的（诚实清单）

1. **默认路径（`surface_at_time`）的音画同步没有测。** 不是没测出来，是**测不了**：
   没有 present fence 就拿不到上屏时刻。`av_drift` 在那条路径上是"不可测量 + 理由"。

   > **2026-10-29 更正（P12-D5）**：上面这句的后半段要改。上屏时刻**是可测的**
   > （`dumpsys SurfaceFlinger --latency`，层名必须带 `#id`），D5 已经测了 9 轮。
   > 仍然成立的是前半段：`av_drift` 这个字段量在"交出那一刻"，默认路径没有它。
   这是 C 组留下的最大一条欠账，明确交给 P12-D。
2. **只有 6 秒、只有 1080p30、只有一台 K40、只有一种音轨（AAC 48 kHz 立体声）。**
   长片源的时钟漂移累积、44.1 kHz、5.1 声道、HEVC + 音频组合都没测。
3. **`av_drift` 的分辨率是 0.25 ms 桶。** 报 0.125 ms 时那是桶心，不是精确值。
   要更细得换采样方式，而不是调桶宽 —— 桶宽再小，样本里的调度抖动也不会更真。
4. **`playback_rate` 的容差 [0.98, 1.02] 是拍的**，依据是"3 帧提前量在 6 s 上占 1.7%"，
   不是从 G7 推出来的。片源越短这个容差越松。
5. **`lead_correction_ns` 用 `schedule_lead` 的 p50 一阶修正。** 严格说提前量是爬坡的，
   用稳态中位数修正首末跨度是近似。6 秒片源上误差 < 1 ms，长片源要重算。
6. **音频侧的"人耳确认"没做。** C1/C3 做过，这一轮没做 —— 自动化证据只证明时钟对齐，
   不证明真的出声了。

---

## 八、证据清单

`docs/evidence/p12c/`（K40 `b6e950ee`，schema `kivo.p12c.sync/1`）：

| 文件 | 内容 |
| --- | --- |
| `p12c8-av-cpu-r{1,2,3}-sync.json` | **主证据**：A/V 合并，`cpu_paced` |
| `p12c8-av-cpu-r{1,2,3}.json` | 同轮次的 `kivo.p12c.decode/5`（视频侧） |
| `p12c8-av-at-r1-sync.json` | 默认路径：`av_drift` 不可测量 + 理由 |
| `p12c8-noaudio-cpu-r1-sync.json` | 无音轨：`clock_source = system`，180/180 帧 |
| `superseded/p12c8-av-r1*.json` | **已废弃**：按 `arrive()` 采样的第一版，见 §三 |

**复跑**（默认已是 `surface_at_time`，要测 `av_drift` 必须显式指定 `cpu_paced`）：

```bash
M=/sdcard/Android/data/com.kivo.cinema/files/media

# 主证据：音画偏差可测
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity \
  --es kivo.media $M/av_1080p30_aac.mp4 \
  --ei kivo.pace 1 --ei kivo.drop 1 --ei kivo.fps 30 \
  --es kivo.present cpu_paced --es kivo.evidence p12c8-av-cpu-r1.json"

# 默认路径：av_drift 应当报"不可测量"
... --es kivo.present surface_at_time

# 强制纯视频（A/B 对照）
... --es kivo.audio off
```

**复算**：

```bash
python tools/verify_p12c_sync.py docs/evidence/p12c/p12c8-*-sync.json
```

**片源重建**（仓库里没有 6 GB 级的素材，只有这一条 6 MB 的）：

```bash
third_party/ffmpeg-sdk/bin/ffmpeg.exe -y -i h264_1080p8.mp4 \
  -f lavfi -i "sine=frequency=440:sample_rate=48000:duration=6" \
  -c:v copy -c:a aac -b:a 128k -ac 2 -ar 48000 \
  -movflags +faststart av_1080p30_aac.mp4
```

单测：`kivo_p12c5_pace_tests` **26 例 / 109 断言**，含 5 条 C8 用例
（呈现时刻 lateness 等于音画偏差 / at_time 没有呈现样本 / at_time 的 lateness 是提交提前量 /
分布可独立重放分位数 / 时钟分档逐帧不漏）；`ctest -R kivo_p12c` **5/5**。

---

## 九、C 组收口：G6 与 G7 的状态

| 目标 | 判据 | 状态 |
| --- | --- | --- |
| **G6** 呈现节拍 | 抖动 p99 ≤ 一帧 | ✅ C5 已证（三档帧率），C7 负载下补证 |
| **G7** 音画同步 | 偏差 p99 ≤ 40 ms | ✅ **C8 首次可测**：0.125–0.375 ms（`cpu_paced`）。⚠️ 本节写的"默认路径不可测"**已于 D5 更正**：上屏端的音画偏差可测（`34-p12d5-status.md`），不可测的只是"交出端"的 `av_drift` |

**G7 的成立有条件，条件必须跟着结论一起引用**：
它在"视频被音频时钟闸住、且 `cpu_paced`"时成立；在默认的 `surface_at_time` 下，
本架构只能证明"提交时刻对齐到 3 帧提前量、跨轮 spread 为 0"，不能证明"上屏时刻对齐"。
把这两句话合成一句"音画同步已达标，且默认路径也达标"，就是造假。

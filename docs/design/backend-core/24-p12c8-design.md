# P12-C8 · 证据产出（C 组收口）· 设计

- 版本：V1.0
- 日期：2026-10-29
- 上游：`07-p12-plan.md`（G7 判据）、`15-p12c4-design.md`（音频主时钟）、
  `17-p12c5-design.md`（节拍器）、`23-p12c7-status.md` §八（C8 的字段接口）
- 卡片：`docs/handoff/TASKS.md` → `P12-C8`

---

## 一、这张卡真正要补的是什么

卡片写的是"一份 JSON 说清 C 组全部事实"。看起来像报表工作，**实际不是**，原因一条：

> **到今天为止，本项目从来没有一条运行同时有音频和视频。**

| 已有的东西 | 时钟 | 有没有音轨 |
| --- | --- | --- |
| `kivo_audio_file_probe`（C3/C4） | AAudio 主时钟 | 有音频，**无视频** |
| `PlayerActivity`（C5/C6/C6B/C7） | `SystemClockSource` | 有视频，**无音轨** |

所以 `av_drift` 这个字段**今天没有任何数据源**。它不是"把两边数字拼起来"，
而是"必须先把两边接起来"，拼是最后一步。

由此 C8 分两件事，缺一不可：

1. **接线**：把 AAudio 主时钟接到视频节拍器上（`PacingSetup::clock` 早就留了这个口，
   只是从来没人往里塞过真音频时钟）；
2. **取证**：把同一次运行里的视频计数、音频计数、A/V 偏差写进一份 JSON，
   并且**偏差的 p99 必须能被第三方从同一份 JSON 重算出来**。

---

## 二、判据（照卡片，未放宽）

| 项 | 要求 |
| --- | --- |
| 路径 | `docs/evidence/p12c/<source>.json` |
| schema | `kivo.p12c.sync/1` |
| 必含 | `device`、`codec`、`source`、`frame_count`、`frames_dropped`、`xrun_count`、`clock_source`、`av_drift{p50,p95,p99,max}`、`present_jitter{p50,p99}`、`timestamp_fallback_count`、`completed`、`error` |
| 可复算 | 独立脚本能从 JSON 重算出 p99，**不接受手填数字** |
| 上限 | G7：音画偏差 p99 ≤ 40 ms |

**"可复算"按字面执行**：JSON 里必须有 per-frame 原始样本，不是只有汇总。
理由是汇总值无法自证 —— 一份只写 `"p99": 12.3` 的证据，任何人只能选择相信。
样本在，p99 就变成一个可以复核的算术结果。

---

## 三、`av_drift` 的定义（先说清楚，不然数字会被误读）

取视频每帧的呈现决策时刻：

```
av_drift = (音频时钟位置 − 锚点音频位置) − (视频 pts − 锚点视频 pts)
```

- **正的 = 视频落后于音频**（音频已经走得更远），负的反之；
- 锚点在第一帧建立，两项同时归零。

**这测的是"相对漂移"，不是"绝对偏差"。** 锚点建立那一刻，音频可能已经播了 100–200 ms
（AAudio 起流 + 首帧解码开销），这段常量会被锚点**吸收掉**，不在 `av_drift` 里。

为什么接受这个定义：G7 关心的是"同步会不会随时间走开"，不是"起播瞬间对齐了没有"。
但**必须把它显式写进证据**，否则读者会把 `av_drift.p99` 当成全链路偏差。因此追加两个字段：

| 字段 | 含义 |
| --- | --- |
| `anchor.audio_position_ns` | 建锚时音频时钟的位置 —— 被吸收的常量，直接可读 |
| `anchor.video_pts_ns` | 建锚时视频首帧 pts |

`av_drift` + `anchor.audio_position_ns` 才是完整的事实。少了后者，前者会被读大。

---

## 四、采集点：**`note_presented()` 里的 `lateness` 就是音画偏差**

> 本节在 V1.0 写错过一次，被第一轮真机测量推翻。原文主张"在 `arrive()` 里另记一条
> `av_drift`，因为两条呈现路径只有它都过"。实测证明这个量在 `surface_at_time` 路径下
> **等于提交提前量**（p50 = −99.875 ms = 3 帧，174/180 个样本甚至越过直方图 −128 ms 下界），
> 与音画同步无关。下面是修正后的口径。

### 4.1 代数关系（决定了能不能这么读）

`FramePacer::arrive()` 里：

```
due_at  = mono_now + (pts − anchor_pts − elapsed)
lateness = mono_now − due_at = elapsed − (pts − anchor_pts)
```

`elapsed` 是**音频时钟自锚点以来走过的媒体时间**。所以

```
lateness = (音频已走) − (视频 pts 已走) = 音画偏差
```

**但只在 `elapsed` 与 `mono_now` 取自同一时刻时成立**，也就是在"帧真正交出去"的那一刻：

| 路径 | 帧何时交出 | `note_presented()` 的 lateness | 是不是音画偏差 |
| --- | --- | --- | --- |
| `kCpuPaced` | 睡到 `due_at` 之后立刻 | `present_at − due_at` ≈ 0 | ✅ 是 |
| `kSurfaceAtTime` | 提前 3 帧交，显示交给合成器 | 不被调用（样本为空） | ❌ **无样本** |

`surface_at_time` 路径下唯一能拿到的是 `arrive()` 当时算出来的值，而那时 `elapsed`
还没走到 `due_at`，差值恒等于"提前量"。**它长得像音画偏差，其实不是。**

### 4.2 于是 C8 的做法

- **不新增直方图。** 直接复用 `pace.lateness`，在证据层按 `present_schedule` 判定能不能算；
- `present_schedule == "surface_at_time"` 时写 `av_drift.measurable = false` 并给出理由，
  **不写一堆 0** —— 一堆 0 会被读成"零偏差"，比没有更坏；
- 要想拿到可测量的 `av_drift`，就用 `--es kivo.present cpu_paced` 跑。

这条限制不是本项目的实现缺陷，而是**架构约束**：C6B §二 已经证明在
「纯 NDK + MediaCodec 直出 Surface」下拿不到上屏时刻（`dumpsys --framelayout` 不给数据、
NDK 无 `getFrameTimestamps`）。要真正测上屏同步，得等 P12-D 的 present fence。

### 4.3 那 `surface_at_time` 路径就什么都不测了？

不是。那条路径仍然给出三个可证伪的量：

1. `pace.schedule_lead`：提交提前量是否每帧一样（C7 的判据）；
2. `present_jitter`（`measured_at: "submit"`）：提交间隔的规律性；
3. `timeline.playback_rate`：媒体时间 / 墙钟时间 —— **唯一一个不与节拍器结论循环的检查**，
   它只看墙钟。节拍器整体跑偏时前两个可能依然"完美"，这个会露出来。

`playback_rate` 在 `surface_at_time` 下要减掉提前量爬坡（首帧提前量为 0，稳态是 3 帧，
所以"首末交出时刻之差"比媒体跨度短了整整一个提前量）。修正项单独写进证据
（`lead_correction_ns`），读者可以拒绝它自己重算。


---

## 五、家族树（先定归属，再落文件）

```
backend/
  include_private/video/video_plane/
    core/pace/
      pace_stats.hpp              ← 改：加 av_drift 直方图 + 原始样本 + 回退计数
    backends/mediacodec/session/
      audio_decode_feeder.hpp     ← 移：从 probe/audio 提升（见下）
    backends/avsync/              ← 新家族：只做"音频 + 视频"的合成，不含平台细节
      session/
        audio_playback.hpp        ← 音频播放侧：解封装→解码→环→AAudio→时钟
        av_sync_stats.hpp         ← 合成统计（纯数据，供报表读）
      report/
        av_sync_report.hpp        ← kivo.p12c.sync/1
  src/video/video_plane/
    backends/mediacodec/session/
      audio_decode_feeder.cxx     ← 移
    backends/avsync/
      session/audio_playback.cxx
      report/av_sync_report.cxx
```

**为什么 `audio_decode_feeder` 要搬家**：C8 的音频播放侧要用它，而它现在住在
`probe/audio/`。`backends/` 依赖 `probe/` 是反向依赖 —— 探针是"用完就可以删"的东西，
后端核心一旦依赖它，探针就再也删不掉了。它本就是"mediacodec 解封装 + 音频解码 + 环形缓冲"
的组合，按铁律该在 `backends/mediacodec/session/`。搬过去只改命名空间与 include，
行为零变化（`kivo_audio_file_probe` 同步改 include，C3/C4 的可复跑能力必须保住）。

**为什么合成层叫 `avsync` 而不是塞进 `jni_session_handle`**：
`jni_session_handle.cxx` 已经是 236 行、管着窗口所有权与失败码。再把音频开流、
预灌、起流、时钟切换塞进去，它就会变成"什么都干"的文件，违反单文件单职责。
`avsync` 只认识 `mediacodec` 与 `aaudio` 两棵子树，不认识 JNI。

**为什么 `avsync` 不进 `core/`**：它依赖 Android 的 MediaCodec 与 AAudio。
`core/` 下不许出现平台依赖（`CURRENT.md` §六 第 1 条）。

---

## 六、运行顺序（时序错了会得到假数字）

```
1. create
2. open(video fd, surface)            # 视频解封装/解码器配置好，但还没起跑
3. open_audio(audio fd)               # 音频开流，环灌到 3 个 burst
4. audio.start()                      # 起流
5. 等 clock 拿到首个有效时间戳（上限 2 s）   # 这段是 AAudio 启动开销，24–146 ms
6. set_pacing(clock = audio.clock())  # ← 关键：这里才把音频时钟接进去
7. set_pipeline(...)
8. video.start()                      # 视频开始，节拍由音频时钟驱动
9. 轮询到完成
10. stop 顺序：先停视频供给 → 再停音频（让环排空）→ 最后取时钟统计
```

第 5 步不能省：NDK 明确说 `requestStart` 是异步的，起流瞬间 `getTimestamp` 返回
`INVALID_STATE`。不起流就开视频，前几十帧会读到不推进的时钟 →
`ClockHealth::kFreeRun` → 节拍器切成自生成时间轴，`av_drift` 全变成噪声。

第 10 步顺序也不能反：先停音频会让环里剩的数据永远没人取，被如实记成大量欠载，
把 `xrun_count` 污染成假数字。

---

## 七、失败模式与降级（不许静默）

| 情况 | 行为 |
| --- | --- |
| 片源没有音轨 | **不静默**：`clock_source` 写 `system`，`av_drift` 字段置 `null` 并带 `av_drift_unavailable_reason`；视频照播 |
| 音频协商结果与片源不符（采样率/声道） | **拒绝起播**，带非空 reason。不做重采样（Direct Play First），硬播只会得到音高错误的输出 |
| AAudio 开流失败 | 拒绝，带 reason。**不许**悄悄退回 `system` 时钟继续播 —— 那会产出一份"看起来有 av_drift、其实是节拍偏移"的假证据 |
| 2 s 内拿不到有效时间戳 | 拒绝，带 reason（`first_ok_mono_ns < 0`） |
| 样本超过上限（4096 帧） | 截断并置 `samples_truncated = true`，不许静默截断 |

---

## 八、证据计划

片源：`av_1080p30_aac.mp4`（用仓库内 `h264_1080p8.mp4` 的画面 + 48 kHz 立体声 AAC 合成，
ffmpeg 本地可做），**必须真有一个音轨**，否则整张卡又变成"接线没验过"。

| 轮次 | 目的 |
| --- | --- |
| r1–r3，`--es kivo.present cpu_paced` | `av_drift` 主证据。3 轮看跨轮稳定性 |
| r4，默认 `surface_at_time` | 证明默认路径会如实报"不可测量 + 理由"，且 `playback_rate` 仍可算 |
| r5，无音轨片源（`h264_1080p8.mp4`） | 证明"没音轨"走的是显式降级而不是假数字 |

> 主证据放在 `cpu_paced` 不是回避默认路径，而是 §4.2 的直接后果：
> 只有"帧在 `due_at` 当刻交出"时，`lateness` 才等于音画偏差。

独立复算脚本：`tools/verify_p12c_sync.py` —— 只读 JSON，重算 p50/p95/p99/max 并与
证据内汇总值逐项比对，不一致就非零退出。**这个脚本是 C8 验收的一半**：
没有它，"可复算"只是句口号。

脚本还顺带守四类自洽：桶合计 = 样本数、分位数单调且不越界、
帧数闭合（`released == received`、`rendered + dropped + 1 == received`）、
`clock_source` 与逐帧分档说同一件事。

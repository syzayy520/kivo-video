# P12-F2 设计：seek 后音视频双侧重同步

- 版本：V1.0
- 日期：2026-10-30
- 卡片判据：**30 帧内无花屏/错序，音画不脱节**
- 关联：`44-p12f1-status.md` §六（F1 给 F2 的交接）

---

## 一、判据拆开：三条，各自可测

| 判据片段 | 这里怎么测 | 为什么不用"看起来更直接"的做法 |
| --- | --- | --- |
| 无花屏 | 段内 `samples_rejected=0`、`frames_received == frames_rendered + drops + 1` | 花屏没法从计数里直接看出，但**解不出帧**会；花屏本身只能靠人眼，见 §五 |
| 无错序 | seek 后首帧 pts 落在 `[landing, landing + 1 帧]` 内 | 逐帧比 pts 需要新开一条全量轨迹，代价大且没必要：错序一定先表现在"首帧 pts 不对" |
| 音画不脱节 | **两侧落点之差** `|audio_landing − video_landing| ≤ 40 ms` | **不能用 `av_drift`**：`surface_at_time` 下它不可测（C8 §三），测出来的其实是提交提前量 |

`40 ms` 取的是"一帧 33 ms 向上取整"：两侧各自落到同步帧上，差一帧算同步，差两帧就不算。

---

## 二、必须先承认的三件事

1. **音频侧没有 seek 入口**。`AudioPlayback` 只有 `open/start/stop`；`AudioDecodeFeeder`
   是一条"读到 EOS 就结束"的线程，没有重定位能力。
2. **环缓冲的 `reset()` 只能在两个线程都停下时调用**（C2 定的规矩）。
   所以音频 seek 必须 **停 feeder → 停输出流 → 定位 → flush → 清环 → 重启**，
   不能"边播边清"。
3. **音频时钟是按"已呈现帧数"算的**（C4）。停/启一次 AAudio 流之后帧位置会归零，
   不重锚就会读到 0 —— 表现为视频被节拍器拖到片头。所以 seek 之后**必须显式 rebase**。

---

## 三、做法

### 3.1 音频 seek（新增 `AudioPlayback::seek_to`）

```
feeder_->stop()          // join，环的读者/写者都停下
output_->stop()          // AAudio 流停（不 close：流还要复用，重开方差 24–146 ms 白花）
landing = source_.seek_to(target)   // 音频轨，PREVIOUS_SYNC
decoder_.flush()
ring_->reset()
feeder_->start()         // 重新解码，灌环
wait 预灌（与起播同一水位）
output_->start()         // 重新出音
clock_->rebase(landing)  // 关键：让时钟从新位置起算
```

**为什么 rebase 用 landing 而不是"现在实际的帧位置"**：新流的第 0 帧就是 `landing` 那一段音频，
所以 `media_ns = landing + frames_since_restart`。用 landing 做基准与事实一致；
想更精确就得读新流的首个时间戳，收益 < 1 ms，不值得再加一条平台路径。

### 3.2 时钟 rebase（新增，两个纯/半平台单元）

| 改动 | 位置 | 为什么在这里 |
| --- | --- | --- |
| `AudioClockSource::rebase(media_ns)`（虚函数，默认空实现） | `core/audio/clock/audio_clock_source.hpp` | 节拍器只认这个接口；默认空实现让 `SystemClockSource` 不必假装能 rebase |
| `FramePositionExtrapolator::rebase(media_ns)` | `core/audio/clock/frame_position_extrapolator.hpp` | 外推基准就是"帧位置 ↔ 媒体时间"的映射，只有它能改 |
| `AaudioClockSource::rebase(media_ns)` | `backends/aaudio/clock/` | 同时要重置单调守卫（否则守卫会把新位置钳回旧的更大值 —— **这是最容易漏的一步**） |

> **守卫必须一起重置**：`MonotonicClockGuard` 的职责就是"位置不许倒退"，
> 而 seek 恰恰要倒退。只改外推不改守卫，时钟会**卡在 seek 前的位置**不动，
> 视频被拖住、`av_drift` 爆掉，而所有计数看起来都正常。

### 3.3 视频侧

F1 已经做好，F2 只需要**多记一个事实**：seek 后首帧的 pts。
新增 `DecodePipelineCounters::first_frame_pts_us`（收帧段在第一帧处写一次），
证据里输出 `seek.video_first_pts_us`。

### 3.4 编排顺序（JNI 层）

```
session_seek(handle, target):
  1. 若 audio 存在：audio_result = audio->seek_to(target)
  2. video_result  = session->seek_to(target)          // F1 的路径
  3. session->record_audio_seek(audio_result)          // 事实合并进同一份证据
```

**为什么音频先**：视频的节拍器挂在音频时钟上。先起音频、让时钟先有值，
视频 worker 一起来就能跟着真实时钟走；反过来会先吃到一段"时钟还没动"的帧。

---

## 四、判据（写成纯函数，桌面可测）

`core/seek/seek_budget.hpp` 扩一个 `assess_av_seek()`：

| 条件 | 结论 |
| --- | --- |
| 视频侧的 `assess_seek` 不通过 | 不通过，reason 直接沿用（不重复造） |
| 音频侧 `audio_landing_us < 0` | 不通过："音频没有完成定位" |
| `|audio_landing − video_landing| > 40 ms` | 不通过，reason 写明两个落点与差值 |
| 其他 | 通过；`av_gap_us` 如实记录 |

### 4.1 明确不做

- **不做音频的精确 seek**（到采样点）：`PREVIOUS_SYNC` 对 AAC 也是帧边界，够用。
- **不做 seek 后的淡入淡出**：会掩盖爆音，属于"假装没问题"。
- **不改 AAudio 流的重开策略**：`stop/start` 复用同一 stream，不 `close` 重开。
- **不做连续多次 seek 的合并**（F1 已列为未做）。

### 4.2 家族树

| 新增/改动 | 位置 |
| --- | --- |
| `AudioPlayback::seek_to` + `AudioSeekResult` | `backends/avsync/session/` |
| `rebase` 三处 | `core/audio/clock/`、`backends/aaudio/clock/` |
| `assess_av_seek` + `AvSeekFacts` | `core/seek/seek_budget.{hpp,cxx}` |
| `record_audio_seek` + `seek.audio_*` 字段 | `session/decode_session.*`、`decode_stats_renderer.cxx` |
| `first_frame_pts_us` | `session/decode_pipeline_counters.hpp`、`decode_receive_worker.cxx` |
| JNI 编排 | `jni/jni_session_handle.cxx`（`session_seek` 内） |
| 单测 | `tests/video/core/p12f1_seek_tests/` 追加 `assess_av_seek` 用例 |

---

## 五、风险

| 风险 | 处置 |
| --- | --- |
| 守卫没重置 → 时钟卡在旧位置 | `AaudioClockSource::rebase` 里**必须**同时 `guard_.reset(media_ns)`；单测覆盖"倒退不被钳" |
| 停流期间环被读空 → 记成大量欠载 | seek 前后 `frames_silenced` 会涨；证据里**分段说明**（seek 引入的静音不算质量问题），并在诚实清单点名 |
| 音频重启后首帧比视频晚 | 视频节拍器跟音频时钟，会自动等；判据比的是**落点**不是墙钟 |
| 花屏只能靠人眼 | 本轮给出截图（seek 后首帧），并在诚实清单写"花屏未自动检测" |
| 片源没有音轨 | `audio_result.ok=false`，判据写"无音轨 ⇒ 本条判据不适用"，**不冒充通过** |

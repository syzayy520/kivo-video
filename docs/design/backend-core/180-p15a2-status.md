# P15-A2 状态卡 · 跨度按段：两个数必须描述同一个窗口

- 日期：2026-11-01
- 设计卡：`179-p15a2-design.md`
- schema：`kivo.p12c.sync/42` → `/43`
- 结论：**DONE**。seek 轮的 `media_rate` 从 1.332 / 0.789 回到 **0.999991 / 0.999999**。

---

## 一、一句话

`media_rate` 一直是个**没有窗口的比值**：分子取整场第一帧到最后一帧的 pts，
分母取墙钟真实跨度。没 seek 时两者同窗（1.000）；一旦拖过进度条，
分子把"被跳过的 20 秒"算成播过的时长 —— 于是 1.332 / 0.789。
这一卡把「这一段走了多远」的六个持有者在锚点重建时一起重启，
并出版 `span_segments` / `media_rate_scope`，让每个比值都能说出自己描述的是哪一段。

## 二、改法：一条口径原则，不是六个补丁

> **窗口量按段，生命周期量按场。**

| 类 | 成员 | 锚点重建时 |
|---|---|---|
| 窗口量 | 四跨度端点、`span_frames`、`clock_supplied_frames`、`lead_trace`、`wall_offset`、`drift` 段 | **重启** |
| 生命周期量 | `presented`、`dropped`、`catch_up_frames`、各直方图、`episode`、`drift_rebases`、`clock_*_frames` | **不动** |

落地文件（未新增任何文件，除单测）：

| 文件 | 改动 |
|---|---|
| `core/pace/pace_stats.cxx` | `note_anchor_reset()` 重启六个窗口量 + 计 `closed_segments_`；`summary()` 出版 `span_segments` |
| `core/pace/pace_lead_trace.hpp` | `restart()`（帧长不清 —— 那是配置不是观测） |
| `core/pace/pace_wall_offset.hpp` | `restart()` |
| `backends/avsync/report/av_sync_report.cxx` | 出版 `pace.span_segments`、`timeline.media_rate_scope`；schema `/43` |
| `tools/verify_p12c_sync.py` | A2 组 5 条判据；版本闸门 `SCHEMA_WITH_SPAN_SEGMENTS = 43` |
| `tests/video/pace/p15a2_span_segments_tests/` | 新单测 **23 checks** |

## 三、真机三轮（4K？不 —— 1080p30 60 s，门关，cpu_paced）

| 轮 | seek | `span_segments` | 段内帧 / 全场 | `media_rate` | 口径 | G8 `total_us` |
|---|---|---|---|---|---|---|
| 无 seek | 0 | **1** | 1800 / 1800 | 0.999999295 | `session` | —（拒判，reason 非空）|
| 向前 seek 20 s @5 s | 1 | **2** | 1200 / 1348 | **0.999990920** | `last_segment` | 58 982 |
| 向后 seek 5 s @20 s | 1 | **2** | 1680 / 2280 | **0.999999104** | `last_segment` | 52 857 |

**修复前**（`178` §三记录，同一脚本、同一设备）：1.332 / 0.789。

## 四、四个预言，中了三个，第四个变成一条新事实

| # | 预言 | 结果 |
|---|---|---|
| 1 | 无 seek 轮逐位不变，`span_segments = 1` | ✅ 1800/1800、0.999999295 |
| 2 | 两份 seek 轮回到 `1 ± 0.002` | ✅ 0.999991 / 0.999999 |
| 3 | 段内帧数比全场少约 `5 s × fps` | ✅ 前向轮差 148 帧；本片源 30 fps ⇒ 4.93 s，seek 在第 5 秒 |
| 4 | **可能出现**：`lead_trace` 重启后 `verdict` 从 `stepped` 变 `steady` | ✅ **确实是 steady**，`max_step` 1.45 ms（一帧 33.3 ms）|

第 4 条是这一卡最值得记的一件事：**seek 不会造出"钟跳了"的假象**。
前向 seek 那轮，锚点重建让 `clock_elapsed_ns` 跳了 20 s，若基准不跟着切，
`lead_trace` 会报 `stepped / max_step = 20 s` —— 一个"钟坏了"的恐怖结论，
而事实只是我们自己换了时间轴。**现在这三轮可以放心地读**。

## 五、判据与变异

| 编号 | 内容 | 变异打红它的那一条 |
|---|---|---|
| A2-1 | `span_segments` 是整数且 ≥ 1 | M1 整格删掉 |
| A2-2 | `seek.requests > 0` ⇒ `span_segments ≥ 2` | M2（老判据先抓）|
| A2-3 | 段数与 `media_rate_scope` 必须自洽 | M3 2 段说 session |
| A2-4 | 有 seek 时 `media_rate ∈ 1 ± 2%` | M4（老判据先抓）|
| A2-5 | 有 seek 时 `span_frames < presented` | M5 帧数=全场 |

**M2 / M4 是被既有判据打死的，不是 A2** —— 红的理由是"重算得 0.999991"。
这不是判据的失败，而是一条该写下来的分工：
**数值错了老判据会红；口径错了只有 A2 会红。** 三份 seek 证据当年就是这样，
数值被记成"黑天鹅"，没有人说得出"这两个数描述的是不同窗口"。
变异 6/6 符合预期（含 M6「3 段且自洽」应存活）。

## 六、诚实清单

1. **只跑了 1080p30 / 60 s / 一门一档**。4K、HEVC10、`surface_at_time`、门开四组都没跑 seek。
2. **连续两次 seek 没测过**。代码路径上 `closed_segments_` 会累加（单测覆盖 3 段），
   但真机只跑了 1 次 seek 的轮次。
3. **`media_rate` 现在只描述"最后一段"**。这是口径变更，不是缺陷：
   一段 5 s 的尾段算出来的速率，方差比 60 s 整场大得多（±2% 那条线在短段上更松）。
   `KNOWN_OUT_OF_BAND` 里 5 份历史越界全是无 seek 轮，不受影响。
4. **A2-4 与老判据重叠**（见 §五），它现在的价值是"读完 A2-2/A2-3 之后顺手确认一次"。
5. **`note_anchor_reset()` 的调用方只查过 `FramePacer::reset()` 一条链**
   （`start` / `seek_to` / `resume_after_seek` 都汇到它）。如果以后有别处直接调，
   段数会变多 —— A2-2 只在 `seek.requests > 0` 时要求 ≥ 2，不会误报，
   但"段数比 seek 次数多"这件事目前没有判据守。
6. **G8 现在可以结题了，但只结了 1080p30**：43.5 / 45.8 ms（旧）→ 58.9 / 52.9 ms（本轮），
   预算 150 ms，余量 2.5–2.9×。4K10 的 seek 从未测过。

## 七、下一张卡

**P15-B · G5 零拷贝的计数**（`177-p15-probe-plan.md` 的顺序：A → B → C → E → D）。
理由：G8 现在两张证据都绿、可以结题；G3 已经有数字（302 次，等阈值）；
剩下的 G5 只有"结构上成立"，没有任何计数 —— 而它是"硬解有没有白干"的唯一证据。

**另一条更硬的候选**：`P15-C` 的 302 次分配归因（`make_alloc_facts()` 已按线程分桶，
出版那 8 格就能回答"这 300 次是谁"）。若用户更在意"最强播放器"的体感，
则优先级应让给 **字幕接线**（74 个文件的语义层至今没有像素产出）。

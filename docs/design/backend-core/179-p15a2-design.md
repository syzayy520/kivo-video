# P15-A2 设计卡 · 跨度按段：两个数必须描述同一个窗口

- 日期：2026-11-01
- 前置：`178-p15a-p15c-progress.md` §三（seek 撞出来的真 bug）
- schema：`kivo.p12c.sync/42` → `/43`

---

## 一、起因：一份证据里两个数互相矛盾

P15-A 的三轮真机里，无 seek 那轮 `media_rate = 1.000`，两份 seek 轮是 **1.332 / 0.789**：

| 轮 | `media_pts_span_ns` | `present_wall_span_ns` | `media_rate` |
|---|---|---|---|
| 无 seek | 59.967 s | 59.967 s | 1.000 |
| 向前 seek 20 s | **59.967 s** | **45.017 s** | **1.332** |
| 向后 seek 5 s | **59.967 s** | **76.010 s** | **0.789** |

差的那 20 s 正好是 seek 的距离。**不是播放速度变了，是两个数量的不是同一段时间。**

## 二、根因

`PaceStats` 的四个跨度端点在**整场第一帧**建立（`has_span_` 只立一次），
而 `note_anchor_reset()`（`FramePacer::reset()` 调用，`start` / `seek_to` /
`resume_after_seek` 三条路都走它）**只切了漂移序列的段**：

```cpp
void PaceStats::note_anchor_reset() noexcept {
    drift_.break_segment();      // ← 只有这一行
}
```

于是 seek 之后：`first_pts` 还在 seek 前，`last_pts` 在 seek 后 ——
`pts_span` 把"被跳过的那 20 秒"算成了播过的时长，而墙钟跨度是真实时长。
两者相除就是垃圾。**此前证据目录里 seek 数为 0，这条路径从来没被走过。**

## 三、为什么不能三行修掉

清掉 `has_span_` 会牵动四个读者：

| 读者 | 依赖 | 不清会怎样 | 清了会怎样 |
|---|---|---|---|
| `clock_lead_ns` | `elapsed_span − arrive_mono_span` | 跨段相减，几秒的假领先量 | 段内相减，自洽 |
| `pace_lead_trace` | 自建基准（整场第一帧） | **基准在 seek 前**，与 `clock_lead_ns` 分家 → U1 红 | 必须一起切段 |
| `vs_wall` | `lateness − lead` | 同上 | 必须一起切段 |
| `span_basis` / AA1 | `clock_supplied_frames_ > span_frames_` | 分子按段、分母按场 → 误报 `mixed` | 必须一起切段 |
| T2 恒等式 | `media_span − corrected ≈ clock_lead` | 三项窗口不同 → 假红 | 三项同段，成立 |

所以这一卡定一条**口径原则**，而不是打一个补丁。

## 四、口径原则：窗口量按段，生命周期量按场

| 类 | 判据 | 成员 |
|---|---|---|
| **窗口量**（"这一段走了多远"） | seek / 锚点重建时**必须重启** | 四跨度端点 `first_*/last_*`、`span_frames`、`clock_supplied_frames`、`lead_trace`、`wall_offset`、`drift` 段、`first_lateness` + 两个 `sum_*` |
| **生命周期量**（"整场发生过什么"） | **永不重启** | `presented`、`dropped`、`catch_up_frames`、各直方图、`episode`、`drift_rebases`、`clock_*_frames` |

一句话：**能被"段"这个词修饰的量按段，能被"场"修饰的量按场。**
`presented` 是"这一场交出了多少帧"，`pts_span` 是"这一段走了多远" —
以前两者混在一个作用域里，是这次 bug 的土壤。

## 五、家族树（不新增文件，不新增目录）

| 文件 | 改动 | 为什么放在这里 |
|---|---|---|
| `core/pace/pace_stats.cxx` | `note_anchor_reset()` 补重启窗口量；`summary()` 出版 `span_segments` | 它本来就是节拍账本的唯一持有者 |
| `core/pace/pace_lead_trace.{hpp,cxx}` | 加 `restart()` | 基准是它自己的状态，别人清不了 |
| `core/pace/pace_wall_offset.{hpp,cxx}` | 加 `restart()` | 同上 |
| `backends/avsync/report/av_sync_report.cxx` | 出版 `pace.span_segments` + `timeline.media_window` + `media_rate_scope` | 出版层 |
| `tools/verify_p12c_sync.py` | A2 组判据 | 判据层 |
| `tests/video/pace/p15a2_span_segments_tests/` | 新单测 | 每个判据都要有一个能打死它的用例 |

**不新增 `pace_segment_span.{hpp,cxx}`**：那会把四跨度端点复制成两份
（整场一份、段内一份），而两份端点迟早有一份忘记刷新 —— 这正是
`note_span()` 当初被抽成唯一入口的理由（P14-AA）。**一份状态，一个语义，改作用域不改存储。**

## 六、判据

| 编号 | 内容 | 反例 |
|---|---|---|
| **A2-1** | `/43` 起 `pace.span_segments` 必须是整数且 ≥ 1 | 少出版一格 |
| **A2-2** | `seek.requests > 0` 的轮次必须 `span_segments ≥ 2` | 清端点却忘了计数 |
| **A2-3** | `span_segments == 1` ⇒ `media_rate_scope == "session"`；`≥ 2` ⇒ `"last_segment"` | 段数是 2 却说是整场 |
| **A2-4** | `span_segments ≥ 2` 且速率可测时，`media_rate` 落在 `1 ± 0.02` | 就是本卡要修的那个 1.332 |
| **A2-5** | 有 seek 的轮次，`span_frames < presented`（段内帧数必然少于全场） | 段没真切 |

**A2-4 与既有判据重叠，这件事要写下来**：老判据会拿跨度反算 `media_rate`，
所以"数值错"本来就会红 —— 变异 M2/M4 证明了这一点（红的理由是"重算得 0.999991"，
不是 A2）。A2 组的独立价值在 **A2-2 / A2-3 / A2-5**：它们判的是**口径**，
而口径错了，老判据一个都不响（三份 seek 证据当年就是这样：数值被记成"黑天鹅"，
没人说得出"这两个数描述的是不同窗口"）。

A2-5 是这一组的反向判据：只查 A2-4 的话，一个"永远报 1.0"的实现也能过。

**没有出版 `media_window`**：段内跨度就是 `pace` 那三个跨度本身，
再印一份"段内跨度"等于把同一个数写两处（P14-AA 抽 `note_span()` 时踩过）。

## 七、预言

1. 无 seek 轮 **逐位不变**：`span_segments = 1`，`media_rate` 与 `/42` 完全一致。
   （`reset()` 在起跑时调，那时一帧都还没喂 —— 重启是空操作。）
2. 两份 seek 轮的 `media_rate` 回到 `1 ± 0.002`，`span_segments = 2`。
3. 前向 seek 20 s 那轮，段内帧数会比 `presented` 少**约 5 s × fps** 帧
   （seek 发生在第 5 秒，不是第 20 秒 —— 20 s 是落点）。
4. **可能撞出一条新事实**：如果 `lead_trace` 重启后 `verdict` 从 `stepped` 变 `steady`，
   说明原来那个"阶跃"是 seek 造成的锚点跳变，不是钟跳 —— 那 P14-Y 的若干结论要重读。

第 4 条是这一卡最大的风险，也是它最大的价值：**它会把"seek 造成的假阶跃"和"钟真的跳了"分开。**

## 八、不做的事

1. **不改 `av_drift` 的分位数** —— 那些是逐帧 `lateness` 的分布，与跨度无关。
2. **不改门限 / 不带新的阈值** —— A2-2 的 ±2% 用的是既有 `KNOWN_OUT_OF_BAND` 同一条线。
3. **不追溯 `/42` 及以前** —— 老证据没有 `span_segments`，判据按版本门放行。
4. **不为 seek 设计新的速率口径（如"只统计最后 5 秒"）** —— 那是产品判断，要单独一张卡。
5. **不动 `present_jitter.worst` 的 max/max-abs 之争**（P14-AH 待拍板，另一条线）。

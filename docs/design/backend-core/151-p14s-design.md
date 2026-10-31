# P14-S 设计卡 · 「播放速率」的分子不是媒体时间

> schema：`kivo.p12c.sync/29` → `/30`
> 一张卡只答一个问题：**`timeline.playback_rate` 这个数，分子到底是什么？**
> 答案：**不是 pts 跨度，是「帧数 × 名义帧时长」。而同一份报告里就有一个真正的 pts 跨度。**

---

## 一、起因：判据说它判不动 189 份

`148-p14q-status.md` §七.2 与 `150-p14r-status.md` §七.5 都留了同一条：

> `verify_p12c_sync.py` 只认 `/1`（P14-R 之后加到 `/29`）。四项闭合式
> （`rendered + dropped_late + dns + EOS == received`）在 `/2`–`/28` 的 187 份证据上
> **没有任何人判过**。

于是先把闸门放开试了一遍（不改仓库，只在内存里改 `SUPPORTED_SCHEMAS`）：
**216 份里 201 份通过，15 份失败，15 份全部失败在同一条判据上 —— 播放速率。**

那 15 份分两族：`p13i/j/k/l/m/n/o` 的 seek 轮（0.62–0.96）与 `p13q/r/s/v/ad` 的 4K60 轮
（1.02–1.06）。0.645 这种数不可能是"放慢了"，于是去看分子。

## 二、发现：分子是乘出来的

`av_sync_report.cxx`（本卡之前的原文）：

```cpp
const std::int64_t handed_off = video.frames_rendered + video.frames_dropped_late +
                                video.frames_discarded_no_surface;
const std::int64_t video_span_ns =
    (handed_off > 1) ? (handed_off - 1) * pace.frame_duration_ns : 0;
const std::int64_t wall_span_ns = pace.last_present_mono_ns - pace.first_present_mono_ns;
const double playback_rate = video_span_ns / corrected_span_ns;
```

`video_pts_span_ns` **从来没有量过任何一帧的 pts**。它是帧数减一乘以名义帧时长。
对 **216 / 216** 份证据逐份复算，恒等式 `video_pts_span_ns == (handed_off − 1) × frame_duration_ns`
全部成立，一份不差。

也就是说：分子里的两个因子（帧数、名义帧时长）**都来自节拍器自己**。
这条检查在多处注释里被写成"本项目唯一一个**不与节拍器结论循环**的检查"——
而它其实是循环的：它只是把节拍器的帧数换算成秒，再除以墙钟。

### 2.1 而真正的 pts 跨度早就在同一份报告里

P13-R 为了另一条恒等式（`due_span == arrive_mono_span + pts_span − elapsed_span`）
已经出版过一份**实测**跨度：

```cpp
// pace_stats.cxx:212
out.pts_span_ns = last_pts_ns_ - first_pts_ns_;
```

它从 `/16` 起在场，**115 份**证据里有。于是同一份 JSON 里有**两个都叫「pts 跨度」的数**：

| 键 | 是什么 | 谁在用它 |
|---|---|---|
| `timeline.video_pts_span_ns` | `(帧数−1) × 名义帧时长` | `playback_rate` 的分子 |
| `pace.pts_span_ns` | 首帧到末帧的**实测** pts 差 | P13-R 的四跨度恒等式 |

前者借了后者的名字，读者看到 `video_pts_span_ns` 不会想到它是乘法。这是 P14-P 那个形状
（同一笔账印两遍、只有一遍带误差棒）的第二次发作，只不过这次是**同一件事印了两个不同的数**。

### 2.2 用它重算，P14-R 的豁免完全没必要

`150-p14r-status.md` §三.3 写着：打断轮的速率 `1.0316` 顶穿上界，所以"上界对非 `completed`
不成立"，代码里据此删掉了上界（只留下界）。理由是
`(received−1)/(presented−1) = 196/190`。

用实测跨度重算那一份：

```
pace.pts_span_ns      = 6 333 333 000      （191 帧，实测）
corrected_wall_span   = 6 333 166 196
media_rate            = 1.000026           ← 正中间
```

**打断轮跑得一点都不快。** 1.0316 是分子多算了 6 帧（`dns` 那 6 帧从未上屏，却按名义时长
进了分子）。P14-R 那条"上界对非 completed 不成立"是**被错分子逼出来的结论**，
不是判据的性质。本卡把它删掉，换成"用实测跨度，上界照样成立"。

这是本系列第二次出现"上一张卡的诚实条款其实是一个度量缺陷"（第一次是 P14-Q 推翻
P14-P 的"1716 帧凭空消失"）。

### 2.3 剩下的 4 份是真的越界

换什么分子都越界，且都在 8 秒的 4K60 轮次上：

| 证据 | 旧式（名义） | 新式（实测） | 丢帧 | 墙钟 |
|---|---|---|---|---|
| `p13r-4k60-r3` | 1.056475 | 1.056454 | 37 | 7.557 s |
| `p13ad-rate50-off-r1` | 1.043374 | 1.043353 | 0 | 7.652 s |
| `p13s-4k60-r1` | 1.032032 | 1.032011 | 18 | 7.736 s |
| `p13v-8x-r2` | 1.021176 | 1.021155 | 7 | 7.818 s |

两种分子的差只有 160 µs（一帧的千分之一），所以换分子救不回来：**媒体真的走了 7.983 s，
墙钟只走了 7.5–7.8 s**。这是"帧提前交付"的节拍照，不是账目错。本卡**不解释它们的成因**，
只把它们记进已知越界表（见 §五）。

## 三、改法

### 3.1 报告层（`sync/30`）

`timeline` 块：

| 键 | 变化 |
|---|---|
| `video_pts_span_ns` | **改名** `nominal_span_ns`（值不变，名字不再说谎） |
| `playback_rate` | **改名** `nominal_rate`（同上） |
| `media_pts_span_ns` | 新增：`pace.pts_span_ns`（实测，可为负 —— seek 回退时末帧早于首帧） |
| `media_span_frames` | 新增：`pace.clock_supplied_frames`（这份跨度覆盖了几帧） |
| `media_rate` | 新增：`media_pts_span_ns / corrected_wall_span_ns` |
| `media_rate_measured` | 新增：`media_span_frames >= 2` 且 `corrected_wall_span_ns > 0` |
| `media_rate_reason` | 新增：不可测时说清是哪一格缺 |

**核心一个字没改。** `pace.pts_span_ns` 与 `clock_supplied_frames` 都是现成的
（P13-R 为四跨度恒等式加的），本卡只是把它们从"另一条恒等式的中间量"提到
"速率的分母搭档"。这与 P14-O 的做法一致：能在报告层解决的不动核心。

`media_rate_measured` 带 `_reason` 而不是像 `undelivered_frames` 那样裸着：
因为它是**测量**（两个可能缺席的跨度相除），不是三个计数器的差。
`surface_at_time` 档实测 `clock_supplied_frames = 0`（6 份 attime 证据全是 0），
此时四跨度全 0 —— 正是 R1 那条"接线自证"警告的假观测。

### 3.2 判据层

1. `SUPPORTED_SCHEMAS` 放开到 `/1`、`/3`–`/29`、`/30`（`/2` 从未存在过）。
2. **改名必须一并改**：`/30` 上旧名在场 → 红（半更新的报告不许混进来）。
3. **新增恒等式判据**：`nominal_span_ns == (handed_off − 1) × frame_duration_ns`，
   对 `/1`–`/30` 全判。这一条今天是**所有人都在依赖、却没有任何人检查**的关系。
4. **速率改判实测跨度**：`media_rate ∈ [0.98, 1.02]`，前置 `media_rate_measured`。
   不可测 → 判「不可判」并计数，**不判通过**。
5. **删掉 P14-R 的打断豁免**（`end_reason != completed` 时不判上界）。
6. 已知越界表：4 条，带文件名 + 理由 + 卡号；**表里任何一条不再越界 → 红**
   （防止豁免腐烂成装饰）。

### 3.3 老版本的处置

`/1`–`/15` 没有 `pace.pts_span_ns`（101 份）：`media_rate` 判「不可判——这一版没存实测跨度」。
`/16`–`/29` 的 6 份 attime 轮：`clock_supplied_frames = 0`，同样不可判，但理由不同。

**这两种「不可判」都不等于通过**，且必须在汇总里分开计数。上一张卡立的规矩：
判据的三种结局（通过 / 失败 / 不可判）要各自成行，否则"没有红"会被读成"全好"。

## 四、预言（真机核对）

一轮 1080p30 60 s，`cpu_paced`，`/30`：

1. `nominal_span_ns` 与 `media_pts_span_ns` 相差 **< 1 帧**（33.4 ms）。两者本就该近似相等 ——
   名义时长按 `derived_fps` 算，实测按真实 pts 算，60 s 上的差只该来自取整。
   若差出几十毫秒，说明 `derived_fps` 是错的。
2. `media_rate ∈ [0.98, 1.02]`，且 `media_rate_measured = true`。
3. `media_span_frames == presented`（实测跨度覆盖的帧数就是上过屏的帧数）。
4. 重跑 216 份：账目全绿，速率 **109 判得动 / 107 不可判 / 4 越界**。

第 4 条的分母是这么来的：216 − 101（`/1`–`/15` 无实测跨度）− 6（attime 无钟读数）= 109。

## 五、不做的事

1. **不解释 4 份 4K60 为什么快。** 换分子救不回来，说明是真实的交付提前；成因要另一张卡。
2. **不改 4 份证据，也不放宽区间去容纳它们。** 它们进已知越界表，每次跑都被数出来。
3. **不动 `pace.pts_span_ns` 的算法。** 它是 P13-R 四跨度恒等式的一端，动它要重判 R 组。
4. **不把 `/1`–`/15` 的 101 份补录实测跨度。** 那些报告里没有 pts，补不出来 ——
   只能让它们明确地"不可判"，而不是含糊地"通过"。

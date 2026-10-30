# P13-I 设计：窗口的身份 —— `truncated` 说了一件没发生的事，趋势与分位数各说各的一段

> 前序：`71-p13h-status.md` §六.3 留了一条：「`quantiles` 是整场累计 2600 个样本，
> `trend` 只描述最近 102.4 s 的 2048 个点 —— 这两个数在长片源上不同源，
> 只是各自都说了实话」。本卡去处理这句话，顺手撞到旁边一个**真说假话**的字段。

---

## 一、缺陷 A：`truncated` 把"换过时间轴"说成"环形覆盖过"

```cpp
// core/pace/pace_drift_series.hpp，上一版
[[nodiscard]] bool truncated() const noexcept { return total_ > kCapacity; }
```

`total_` 是**整场累计**的接受数，`clear()` 不清它（这是刻意的，理由见原注释）。
而 seek 会走 `clear()`：

```cpp
} else if (media_ns < last_media_ns_) {
    clear();          // size_ = 0，但 total_ 原样留着
    ++segments_;
    begin_segment(media_ns);
}
```

于是"整场 1200 + 1800 = 3000 > 2048"被判成**覆盖发生过**——而环形里躺着的是
第二段的整整 1800 个点，一个都没被覆盖掉。

真机实测（本卡第 2 轮，`p13i-130s-seek-r1-sync.json`，55 s 时 seek 到 30 s）：

| 字段 | 值 | 说明 |
|---|---|---|
| `sampling.accepted` | 3100 | 整场累计（1200 + 1900 附近） |
| `trend.segment_accepted`（新） | 2000 | **本段**接受数 |
| `trend.window_sample_count`（新） | 2000 | 窗口里剩的点 = 本段全部 |
| `trend.truncated`（旧口径会写） | **true** | `3100 > 2048` |
| `trend.truncated`（实际应为） | **false** | 本段 2000 < 2048，一个点都没被覆盖 |

**这份数据自相矛盾**：覆盖过的环形序列必然满 2048。写着 `truncated: true`
却只有 2000 个点，等于说"这个杯子装满了 2048 毫升，现在里面有 2000 毫升"。

### 为什么这个假话从没被人看见

全部 33 份带 `trend` 的 sync 证据里，**32 份 `segments == 1`**，剩下那份是本轮的 seek 轮。
一次都没在 seek 之后取过 sync 报告 —— P12-F1 的 seek 取证走的是 `decode/` 那条线，
根本不经过节拍器的序列。所以这不是"一直在出错"，是"从来没被问到过"。
**一个从没被问到过的字段，它的名字就是它唯一的规格。**
而它的名字（`truncated`）与它当时实现的语义（累计数越界）差得很远。

### 危害不是"多了一个 true"

`truncated` 同时被当成两个问题的答案：

1. 环形的旧点被吃掉了没有？（窗口为什么比累计少）
2. 这份趋势描述的是整场还是最后一段？

第 2 个问题**根本不是它能回答的**，答案在 `segments` 里。上一版把两件事压在一个
布尔上，于是只可能出现两种状态："对了一个、错了一个"，或者"两个都错"。
判据 I3 的作用就是把这个布尔重新绑回第一个问题。

---

## 二、缺陷 B：分位数与趋势是两批样本，却并排摆着

`quantiles.media_time_decimated` 来自 `PaceStats::sampled_lateness_`，
**整场累计**：环形覆盖之后继续加，seek 之后也继续加。
`trend` 来自 `DriftSeries`，**只描述窗口**（封顶 2048 点 / 102.35 s）。

两份都说了实话，`trend.truncated` 也如实写了 true。问题在**摆法**：

```json
"quantiles": { "media_time_decimated": { "samples": 2600, "p99": 375000, "max": 6518427 } },
"trend":     { "window_sample_count": 2048, "slope_ppm": -0.022, ... }
```

读表的人把 `p99` 和 `slope` 并排抄进同一行时，不会想起去看三行之外的 `truncated`。
**"记得看注释"不是保障**——这是 P13-E 用一整张卡换来的教训，修法从来不是提醒，是结构。

本轮第 1 轮实测同一份数据、同一台机器：

| 口径 | 样本 | p50 | p95 | p99 | max |
|---|---|---|---|---|---|
| 整场累计 | 2600 | 0.125 ms | 0.125 ms | 0.375 ms | **6.518 ms** |
| 窗口（最近 102.4 s） | 2048 | 0.125 ms | 0.125 ms | 0.375 ms | **2.040 ms** |

p50/p95/p99 三个数**恰好相同**，只有 `max` 差 3.2 倍 —— 最差的那一下发生在
被覆盖掉的头 552 个点里。**"恰好相同"是最坏的情况**：它让两份数据看起来同源，
于是没人会去查它们到底是不是同一批。

（要不要干脆把分位数也改成窗口口径？不要。两个问题都是真问题：
"整场有多差"必须覆盖整场，否则长片源上越跑越"好看"；
"这段漂得怎么样"必须只覆盖被回归的那段，否则斜率描述的不是它自己的样本。
所以两份都留，但**各自带上自己的样本**。）

---

## 三、改法

### 3.1 `truncated` 改判段内，段内计数单独维护

```cpp
// pace_drift_series.hpp
[[nodiscard]] std::int64_t segment_samples() const noexcept { return segment_total_; }
[[nodiscard]] bool truncated() const noexcept { return segment_total_ > kCapacity; }
```

`segment_total_` 的**唯一写入点**是 `clear()`（环形清空，计数跟着归零）
与 `note()` 的接受末尾的 `++`。`begin_segment()` 里**不**清零 —— 两处写同一个数
早晚不一致；而进入它的两条路（新建对象、seek 分支）都先过 `clear()`。

归零放在 `clear()` 而不是只放在"新段开始"，是为了让

```
size() == min(segment_samples(), kCapacity)
```

在**任何时刻**成立，包括"clear 之后还没进新帧"那个空活段的瞬间。不归零的话，
那一刻会同时出现 `size() == 0` 与 `truncated() == true` —— 正是本卡要消灭的自相矛盾。

### 3.2 趋势自带分布

新增一个单元 `core/pace/pace_drift_window.{hpp,cxx}`：

```cpp
struct DriftWindow { SignedHistogram lateness{}; std::int32_t samples = 0; };
[[nodiscard]] DriftWindow summarize_window(const DriftSeries& series) noexcept;
```

它只做一件事：把序列里还留着的点重放成一份分布。`PaceStats::summary()` 调它一次，
结果挂在 `PaceSummary::window_lateness`。报告层把它渲染成 `trend.window_quantiles`。

为什么单开一个文件，而不是塞进 `pace_drift_trend.cxx`：那个文件是"回归"，
这个是"分布"。混在一起之后，`DriftTrend` 迟早会被人塞进一个 8 KB 的直方图成员，
而它的头文件写着"尺寸刻意压到 ~400 字节"（`PaceSummary` 会整块复制）。

为什么 `DriftWindow` 里还要一个 `samples`（明明 `histogram.count()` 就是它）：
JSON 里这个数必须写在分布**前面**、紧挨 `segment_accepted`。指望读者越过
1024 个桶的数组去核对样本数，是不现实的。

### 3.3 `segment_samples` 一删为二

```
segment_samples  →  segment_accepted     （本段接受数）
                 +  window_sample_count  （窗口里剩几个 = min(上者, capacity)）
```

旧名字同时被读成"这段采了几个"和"窗口里剩几个"——**它两个意思都用过**，
而这两个数在 seek 之后相差 1100。含糊的名字比缺字段更坏：缺字段会被发现，
含糊的名字会被两个方向各读一次，然后两边都觉得自己对。

### 3.4 schema 升 `/7`

`/6` 及以前的 `truncated` 是整场口径。不重写旧证据（重写等于伪造），
I 判据只对 `/7` 成立。

---

## 四、判据 I0–I6

| 编号 | 判据 | 它抓什么 |
|---|---|---|
| **I0** | `capacity` / `segment_accepted` / `window_sample_count` 必须在；`segment_samples` 必须已删除；`capacity` 与 C++ 常量一致 | 缺一格就要去别处找；旧名字回来了 |
| **I1** | `Σ window_samples == window_sample_count` | 分窗循环（8 趟）与序列长度（`size()`）数的不是同一批点 |
| **I2** | `window_sample_count == min(segment_accepted, capacity)` | 报告层的算法本身，用 JSON 重算一遍 |
| **I3** | `truncated ⇔ segment_accepted > capacity` | **本卡的核心**：覆盖是每段的事，不是整场的事 |
| **I4** | 窗口 max ≤ 整场 max（子集）；**未覆盖且单段时两份分布逐桶相同** | 两份分布不同源；或者"不同"却给不出原因 |
| **I5** | 覆盖 ⇒ 窗口满 ∧ 段内 > 容量；未覆盖 ⇒ **段内** ≤ 容量 | 那句"装满了 2048 却只有 2000"的自相矛盾 |
| **I6** | 单段 ⇒ `segment_accepted == accepted`；跨段 ⇒ `≤` | 两个 accepted 有一个算错了段 |

I1 与 I4 是本卡唯一"两条独立代码路径互相印证"的地方：
`window_samples` 由 8 趟分窗循环数出来，`window_sample_count` 由 `size()` 给出；
窗口分布由 `summarize_window()` 重放序列，整场分布由 `note_presented()` 逐帧累加。
**两边都对，才说明序列真的留住了那批点。**

---

## 五、取证

| 轮次 | 片源 | 触发 | 要看什么 |
|---|---|---|---|
| r1 | 130 s / 360p30 | 无 seek，2600 点 > 2048 | 覆盖路径：`truncated: true`、窗口满 2048、两份分布**不同**（max 差 3.2×） |
| r2 | 130 s / 360p30 | 55 s 时 seek 到 30 s | **缺陷 A**：`segments: 2`、`truncated: false`、段内 2000、整场 3100 |
| r3 | 35 s / 1080p30 | 不长，700 点 | **I4 的身份判据**：两份分布逐桶相同 |

r3 是"证同一件事"，r2 是"证不同一件事"。只做 r1 会以为窗口永远等于整场；
只做 r2 会以为窗口永远不等于整场。**两条路都要有一份真机证据**，
因为它们各自对应一个判据分支。

r2 的关键不是"红了"，是**同一份数据，旧判据会写 true，新判据写 false**，
而 I5 能从"窗口没满"当场把这个 true 判成假。

---

## 六、诚实边界（先写明）

1. **这一轮的 p50/p95/p99 两份恰好相同。** 只有 `max` 不同。
   所以本卡不会让任何一张已有的分位数表变样 —— 它改的是"读者能不能发现两份不同源"，
   不是"数本身算错了"。**恰好相同不是同源。**
2. **`truncated` 是语义变更，不是纯新增。** `/6` 及以前这个字段在 `segments > 1` 时
   给的是错的答案。历史上没有一份 sync 证据触发过它（32 份全 `segments == 1`），
   所以没有"旧证据要重读"的问题 —— 但这只是运气，不是设计。
3. **只有 1 轮 seek，且只 seek 一次。** 两段是 2；三段以上、连续 seek、
   seek 到自己前面再倒回去，都没测。`segment_total_` 的归零在多次 `clear()` 下
   靠单测 `t03`/`t04` 钉住，没有真机覆盖。
4. **窗口分布让 `PaceSummary` 大约多 8 KB**（一个 `SignedHistogram`）。
   只在 `stop()` 之后算一次，稳态零开销；但它确实让"整块复制的结构"变大了。
5. **`window_quantiles` 没有 `worst`。** 累计口径已经有一个 `worst`；
   再给窗口一个同名不同口径的，就是 P13-E 花一整张卡消灭的那种东西。
   想知道"最差那一下在哪儿"，去累计那一份里看，它带着 `media_ns`。
6. **漂移量准仍是 7 ppm 的尺子。** 三轮 `slope_from_windows_ppm` 全是 0.000、
   `r2 ≈ 0`，只能说"小于这把尺子的分辨率"。这条从 P13-D 欠到现在，一个字没动。
7. **判据自己会犯它要抓的错。** 见 `73-p13i-status.md` §六：H2 的 `+1` 只加了一次、
   I5 的第一版拿整场容量去比段内 —— 两条都是"把整场当成一段"，也就是本卡的主题。

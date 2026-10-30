# P13-G 设计：抽稀的最小可用门槛 —— 一条不可达的降级分支，与一份只有 1 个样本的"抽稀"

> 前序：`65-p13e-status.md` §六.1 把"`per_frame` 降级分支缺真机证据"列为欠账，
> `67-p13f-status.md` §六.1 原样继承。本卡去看这条欠账，结论是**它不是欠证据**。

---

## 一、问题：欠的不是证据，是路径

`65-p13e-status.md` 写的是：「`note_presented()` 拿不到媒体时刻的那条路只有单测覆盖。
要造它得让呈现路径不传 pts，本轮没做。」

这句话隐含一个假设：**存在一条"不传 pts"的路径，所以只要造出它就能取证。**

去看代码，这个假设不成立。

`PaceStats::note_presented(lateness_ns, media_ns)` 是**唯一**往 `lateness_` 里加样本的地方
（`pace_stats.cxx:18`），也是唯一调 `drift_.note()` 的地方（`:28`）。两者在同一个函数体里，
中间只隔一个 `drift_.note()` 的返回值判断。而 `DriftSeries::note()` 的首帧分支
**无条件 `return true`**（`pace_drift_series.cxx:10-15`）。

于是有一条链：

```
drift_measurable
  ⇒ pace.lateness.count() > 0        // av_sync_report.cxx:216 的第四个条件
  ⇒ note_presented() 至少被调过一次
  ⇒ drift_.note() 至少被调过一次
  ⇒ 首帧必被接受
  ⇒ sampled_samples >= 1
  ⇒ decimated == true
```

`sampled_samples == 0 && drift_measurable` 这个组合**在结构上不可能出现**。

而 `kPerFrame` 分支的进入条件正是 `drift_measurable && !decimated`
（`av_sync_report.cxx:236`），所以：

**`SamplingMode::kPerFrame` 在报告层是一条死路径。P13-E 为它写的那句 reason ——
`"no media pts was supplied to note_presented()"` —— 描述了一件不可能发生的事。**

单测能覆盖它，是因为单测直接构造 `PaceSummary` 绕过 `PaceStats`。
这不是"覆盖了"，是"测了一个报告层永远走不到的分支"。

---

## 二、真缺陷：缺 pts 不会降级，会静默产出一份假抽稀

`kPerFrame` 不可达，那"缺 pts"时实际发生什么？

`media_ns` 来自 `decision.media_ns = pts_ns`
（`frame_pacer.cxx:115`），上游是 `frame.presentation_time_us * 1000`
（`decode_receive_worker.cxx:111`）。而 `DecodedFrame::presentation_time_us`
的默认值是 **`0`**（`decoded_frame.hpp:16`），不是 `-1` 也不是哨兵值。

**「缺 pts」和「第一帧的 pts 真的是 0」在类型上无法区分。**

若一整条流都缺 pts，所有 `media_ns == 0`：

| 帧 | `media_ns` | `DriftSeries::note()` 走哪个分支 | 结果 |
| --- | --- | --- | --- |
| 第 1 帧 | 0 | ① 首帧 | 接受，`next_due = 50 ms` |
| 第 2 帧起 | 0 | ③ `media < next_due` | 全部 `skipped` |

于是 `sampled_samples == 1`、`span_ns == 0`。报告层会输出：

```jsonc
"media_time_decimated": {
  "samples": 1,
  "p50": 12345, "p95": 12345, "p99": 12345, "max": 12345,   // 四个数完全相等
  "worst": { "index": 0, "of": 1, ... },
  "histogram": { ... }
}
```

**四个分位数完全相等**是它唯一的破绽，但没有任何判据在看这件事。
反过来，下面这些判据会**全部通过**：

| 判据 | 为什么抓不住 |
| --- | --- |
| E1 顶层无裸分位数 | 结构完全合规 |
| E2 指针可解析 | `cross_frame_rate_set` 指向一个真实存在的块 |
| E3 有 accepted ⇔ 有抽稀那份 | 有 1 个样本，条件成立 |
| F1 `worst` = `max` | 逐位相等，成立 |
| F2/F2′ 位置自洽 | `index 0 × 50 ms = 0 ms == media_ns 0`，成立 |
| `trend.usable` | 会是 `false`（< 30 样本），但**它只是一句附言**，分位数照样在 |

**这是比"死分支"更坏的一种：它长得像正常数据。**
`67-p13f-status.md` §五.4 已经记过一条同类的："不可测时 `quantiles` 是空对象，
所以 F3 的防御分支不可达" —— 那条结论在**抽稀整块缺席**时成立，
但没考虑到"抽稀块存在而只有一个样本"这个中间态。

### 为什么不是"极端假想"

P13-E 自己撞过一次近亲：`Report(Path("D2 media_time_decimated/p99 …"))`
被 `Path.name` 吃掉口径前缀。那一类错的共同点是**局部合法、整体无意义**。
`samples: 1` 属于同一族：每个字段都对，加起来是一句空话。

---

## 三、修法：一个门槛，收在 `sampling_mode` 里

### 3.1 门槛的形状

抽稀块要成立，至少要能证明"媒体时间在**推进**"：

```
sampled_samples >= kMinDecimatedSamples   // = 2
且 span_ns > 0
```

选 2 而不是 30，理由是把两种门分清楚：

| 门 | 管什么 | 阈值 | 在哪 |
| --- | --- | --- | --- |
| **结构门**（本卡新增） | 这份数据**是不是抽稀数据** | 2 个样本 / 跨度 > 0 | `sampling_mode` |
| **统计门**（已有，P13-D） | 这份数据的**斜率可不可信** | 30 个样本 / 30 s | `DriftTrend` |

一个 1.2 s 的片源只有 24 个抽稀点：**它确实是抽稀数据**（结构门该过），
**但它的斜率不可信**（统计门该拦）。用同一个阈值管两件事，会让
"短片源的 p99 有意义但斜率没有"这个真实情况被一刀切掉。

### 3.2 门槛放在哪一层

放进 `core/pace/sampling_mode` 单元，与口径名同处。理由：

- 它是**口径判定**的一部分（"这个口径在这一轮成不成立"），不是渲染细节；
- 报告层若自己写 `samples >= 2`，就会和 `sampling_mode_name()` 一样出现第二份真相；
- `p13e_sampling_mode_tests` 已经在那儿钉名字，加一条函数比新建单元便宜。

新函数：

```cpp
/// 抽稀块是否成立。`samples` 与 `span_ns` 必须来自**同一份**统计。
[[nodiscard]] bool decimated_block_is_usable(std::int32_t samples,
                                             std::int64_t span_ns) noexcept;

/// 抽稀不成立时的**真实**原因。给 `sampling.reason` 用。
[[nodiscard]] const char* decimated_unusable_reason(std::int32_t samples,
                                                    std::int64_t span_ns) noexcept;
```

第二个函数存在的理由，是本卡第一节那条教训的直接产物：
`kPerFrame` 的 reason 之所以敢写"没传 pts"，是因为**没有一个地方负责回答"为什么"**，
于是渲染层随手编了一句。把"为什么"收成函数，编造就没有位置。

三种返回：

| 条件 | reason |
| --- | --- |
| `samples == 0` | `"no decimated sample was accepted"` |
| `samples == 1` | `"media time did not reach the second 50 ms grid point (single sample)"` |
| `span_ns <= 0` | `"media time does not advance across the accepted samples"` |

### 3.3 报告层怎么变

`kPerFrame` 从死路径变成**真正会被走到的降级**，且理由是真话：

```cpp
const bool decimated_usable =
    decimated && core::pace::decimated_block_is_usable(pace.sampled_samples,
                                                       pace.drift.span_ns);
if (!drift_measurable) {
    mode = kNotMeasured;
    ...
} else if (!decimated_usable) {
    mode = kPerFrame;
    sampling_reason = decimated_unusable_reason(samples, span);
    // "只有逐帧分位数，且不可跨帧率比较"
}
```

`quantiles.media_time_decimated` 的渲染条件从 `if (decimated)` 改成
`if (decimated_usable)`。**不改数字，只是不再假装有。**

`sampling.accepted` 仍写真实的 `sampled_samples`（可能是 1）。
**"采到了几个"和"这份能不能用"是两件事，都要说** —— 把 accepted 写成 0 才是撒谎。

### 3.4 一个反面选项，和为什么不选它

**选项 B：把缺 pts 变成显式哨兵**（`presentation_time_us = -1` 表示无 pts，
`note_presented` 见 -1 就不采）。

看起来更根本，但有三个问题：

1. `presentation_time_us` 默认 0，改成 -1 要动 `DecodedFrame` 的初值与所有构造点，
   影响面远超本卡，且与 `mediacodec_audio_decoder.hpp:26` 已有的 `-1`
   （那里 -1 表示"未定"，语义不同）容易混淆。
2. 即便有了哨兵，本卡的门槛**仍然需要** —— 一个正常的短片源也可能只有 1 个抽稀点。
   两件事不互相替代。
3. 没有真机证据能证明哪条流会缺 pts。先改哨兵等于**为未观测到的情况改核心结构**。

记进 §五 边界，留给有证据时再做。

---

## 四、判据（G1–G3）

对每一份 `sync/5` 证据：

| 判据 | 内容 |
| --- | --- |
| **G1** | `sampling.mode == media_time_decimated` ⇒ 抽稀块存在，且 `samples >= 2`，且 `histogram.span_ns > 0` |
| **G2** | `sampling.mode == per_frame` ⇒ 抽稀块**整块缺席**，且 `per_frame.samples > 0` |
| **G3** | `sampling.accepted` 与抽稀块 `samples` 自洽：mode 为 decimated 时相等；mode 为 per_frame 时 `accepted < 2` 且抽稀块缺席 |

G1 是本卡的核心：它把"看起来像抽稀数据"和"是抽稀数据"分开。
G2 拦的是"降级了却还留着一份 1 样本的块"——那正是修法要消灭的形态。
G3 拦的是"把 accepted 改写成 0 来让 G2 通过"这条取巧路。

### 负向自测（判据必须先能被打红）

| 层 | 变异 | 期望 |
| --- | --- | --- |
| C++ 单测 | `decimated_block_is_usable` 恒返回 `true` | 门槛测试红 |
| C++ 单测 | 门槛改成 `samples >= 1` | 单样本用例红 |
| Python | 把一份 `mode=decimated` 的证据改成 `samples=1` 但保留块 | G1 红 |
| Python | `mode=per_frame` 却保留抽稀块 | G2 红 |
| 端到端 | 坏证据放进 `docs/evidence/p13g/` 跑总收口 | 层 4 红（**先断言它进了被扫集合**） |

最后一条要按 `67-p13f-status.md` §四.2 的教训做：**先证明文件被扫到，再看红不红**。

---

## 五、边界（写在这里，别在状态卡里第一次出现）

1. **本卡不证明任何一条真实流会缺 pts。** 修的是"如果发生，证据会长成什么样"。
   没有真机复现，也没有为它改 `DecodedFrame` 的初值。
2. **`samples >= 2` 是结构门，不是统计门。** 2 个样本的 p99 仍然不可信；
   它只保证"这不是单点"。可信度仍归 `DriftTrend::kMinSamples` 管。
3. **`span_ns` 取自 `PaceSummary::drift.span_ns`**，也就是抽稀序列的首末媒体时刻差。
   若抽稀块缺席但 `span_ns > 0`（例如全被 `clear()` 后重来），G1 不看它 —— 那不是本卡范围。
4. **不改 `sync/4` 的既有证据。** 26 + 20 份旧证据仍按旧判据读；
   新判据只对 `/5` 生效（与 P13-E 处理 `/2`、P13-F 处理 `/3` 同一做法）。
5. **`kPerFrame` 从此是真可达的**，但本卡**没有**真机证据让它被走到 ——
   要造它需要一个"媒体时间不推进但有逐帧数据"的片源，本卡没造。
   这一条是真正的遗留，且现在它的理由是准的（`decimated_unusable_reason`），
   不再是编的。

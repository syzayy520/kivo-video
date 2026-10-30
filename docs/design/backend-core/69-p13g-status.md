# P13-G 状态卡：那条降级路径从来没被走过 —— 它不可达

> 设计：`68-p13g-design.md`。前序：`65-p13e-status.md` §六.1、`67-p13f-status.md` §六.1。
> 证据：`docs/evidence/p13g/` 10 份（5 轮 × decode+sync），**全部出自同一 APK**，全部 `sync/5`。
> 结论：判据 G1–G3 全通过；总收口 336 份 / 四层通过 / 8 子校验器 exit 0。

---

## 一、欠账不是"缺证据"，是"这条路不存在"

连续两张卡记着同一条遗留：「`per_frame` 降级分支缺真机证据，要造它得让呈现路径不传 pts」。

去看代码，这个欠账的**前提**是错的：

```
drift_measurable
  ⇒ pace.lateness.count() > 0        // av_sync_report.cxx 的第四个条件
  ⇒ note_presented() 至少调过一次     // 唯一往 lateness_ 加样本的地方
  ⇒ drift_.note() 至少调过一次        // 同一个函数体，中间只隔一次返回值判断
  ⇒ 首帧必被接受                     // drift_series.cxx:10-15 无条件 return true
  ⇒ sampled_samples >= 1 ⇒ decimated
```

`kPerFrame` 的进入条件是 `drift_measurable && !decimated`，两个合起来不可能同时成立。

**P13-E 为它写的那句 reason —— `"no media pts was supplied to note_presented()"` ——
描述了一件不可能发生的事。** `note_presented()` 的签名就要求 pts。

单测能"覆盖"它，是因为单测直接构造 `PaceSummary` 绕过 `PaceStats`。
那不是覆盖，是测了一个报告层永远走不到的分支。

---

## 二、真缺陷：缺 pts 不会降级，会静默产出一份假抽稀

`DecodedFrame::presentation_time_us` 默认值是 **`0`**（`decoded_frame.hpp:16`），
不是哨兵值。所以「缺 pts」和「第一帧 pts 真的是 0」在类型上无法区分。

若一整条流都缺 pts，所有 `media_ns == 0`：首帧接受，其后全部 `skipped`。
结果是 `sampled_samples == 1`、`span_ns == 0`，然后报告写出一份这样的块：

```jsonc
"media_time_decimated": {
  "samples": 1,
  "p50": 12345, "p95": 12345, "p99": 12345, "max": 12345,
  "worst": { "index": 0, "of": 1, ... },
  "histogram": { ... }
}
```

**四个分位数完全相同**是它唯一的破绽。而 E1 / E2 / E3 / F1 / F2 / F2′ 会**全部通过**：

| 判据 | 为什么抓不住 |
| --- | --- |
| E1 顶层无裸分位数 | 结构完全合规 |
| E2 指针可解析 | `cross_frame_rate_set` 指向一个真实存在的块 |
| E3 有 accepted ⇔ 有抽稀那份 | 有 1 个样本，条件成立 |
| F1 `worst` = `max` | 逐位相等 |
| F2/F2′ 位置自洽 | `index 0 × 50 ms = 0 ms == media_ns 0` |

`trend.usable` 会是 `false`，但**它只是一句附言**，分位数照样在块里。
读表的人先看到的是 p99。

---

## 三、改了什么

### 3.1 一个门槛，收在 `sampling_mode`

```cpp
inline constexpr std::int32_t kMinDecimatedSamples = 2;   // 结构门
bool decimated_block_is_usable(std::int32_t samples, std::int64_t span_ns);
const char* decimated_unusable_reason(std::int32_t samples, std::int64_t span_ns);
```

**2 是结构门，30 是统计门**（`DriftTrend::kMinSamples`），两者不能共用：
1.2 秒片源只有 24 个抽稀点，它**确实是抽稀数据**（结构门该过），
但**斜率不可信**（统计门该拦）。用同一个阈值管两件事，
会让"短片源的 p99 有意义、斜率没有"这个真实情况被一刀切掉。

`decimated_unusable_reason()` 存在的理由是本卡第一节那条教训的直接产物：
那句假话之所以写得出来，是因为**没有一个地方负责回答"为什么"**，
于是渲染层随手编了一句。把"为什么"收成函数，编造就没有位置。
它被单测逐字钉着，**可用时返回空串**——想写第二个解释，就得先改这里。

### 3.2 报告层

`mode` 判定从 `decimated = sampled_samples > 0` 改成 `sampled_samples > 0 && usable(…)`。
`quantiles.media_time_decimated` 的渲染条件跟着改。
`sampling.accepted` **仍写真实值**（可能是 1）——"采到了几个"和"这份能不能用"是两件事，都要说。
把 accepted 改写成 0 才是撒谎。

`schema` 升 `kivo.p12c.sync/5`。

---

## 四、判据

| 判据 | 内容 | 结果 |
| --- | --- | --- |
| G1 | `mode == decimated` ⇒ 块存在、`samples ≥ 2`、`span > 0` | ✅ 5/5 |
| G2 | `mode == per_frame` ⇒ 抽稀块**整块缺席** | ✅（正例 M4） |
| G3 | `mode == per_frame` ⇒ `accepted` 确实不够格（不是判据与实现不同步） | ✅（正例 M4） |

### 变异自测（9 处，全部打红或按预期通过）

| 层 | 变异 | 结果 |
| --- | --- | --- |
| C++ | `decimated_block_is_usable` 恒返回真 | 9 条红 |
| C++ | 门槛松到 `samples >= 1` | 6 条红 |
| C++ | 可用时也返回"不推进"的理由 | 2 条红 |
| C++ | `note_presented` 加 `media_ns > 0` 门 | 4 条红（**第二版才红，见 §五.1**） |
| Python | `mode=decimated` 但 `samples=1` | 4 条红（含 G1） |
| Python | `mode=per_frame` 却保留抽稀块 | 3 条红（G2 + G3） |
| Python | `mode=per_frame` 但 `accepted=160` | 2 条红（G3） |
| Python | **正例**：真降级、自洽 | **全部通过**（不误伤） |
| 端到端 | 坏证据放进 `docs/evidence/p13g/` 跑总收口 | 层 4 红，结论「不可交付（1 条失败）」 |

端到端这次先做了 P13-F §四.2 要求的那一步：
断言 `docs/evidence/p13*/*-sync.json` 命中 39 份且坏证据在里面（`True`），再跑。

---

## 五、过程里抓到的五个问题

### 1. 我自己的测试第一版假通过

`t07` 用 200 次随机序列守"`lateness.count() > 0` ⇒ `sampled_samples >= 1`"。
把 `note_presented` 改成 `if (media_ns > 0 && drift_.note(...))` 喂给它 —— **没红**。

原因：随机序列的媒体时刻从 0 单调累加，只有"只调一帧且步进恰为 0"才碰得上，
概率约 0.08%，200 次试验期望命中 **0.16 次**。

改成先写一条显式最小反例（一帧、`media_ns = 0`），再拿随机扫别的组合，重跑立刻红。

**随机测试的覆盖 ≠ 断言想覆盖的场景。** 这条和 P13-F 的"测试自己也会假通过"
是同一族，只是伪装不同：那次是文件没被扫到，这次是分布没被采到。

### 2–5. 同一张表只改了一半（本卡撞了四次）

| # | 位置 | 症状 |
| --- | --- | --- |
| 2 | `SCHEMA_WITH_WORST` 从字符串改成元组，判断还是 `==` | F 判据会对 `/4` `/5` **静默全部报"不适用"** |
| 3 | `check_pair` 里硬编码 `schema == "kivo.p12c.sync/3"` | **P13-F 的 20 份 `/4` 证据从来没进过 D1/D2 的配对池** |
| 4 | `observed_hz = accepted / span` 的 else 分支写 `accepted == 0` | 合法的单样本降级被判失败（正例 M4 第一次跑就撞上） |
| 5 | 我把 G2 的调用写在 `per_frame` 赋值之前 | `UnboundLocalError`，编译期没拦住（Python） |

第 3 条是本项目第四次撞"加 schema 忘了改分发"：
P13-E 是 `card_checks` 漏写，P13-F 是层 4 漏目录，本卡是 `check_pair` 漏判据。
本卡的处理不是再补一处，而是**在 `verify_p12_all.py` 顶部定义
`SYNC_QUANTILE_BLOCK_SCHEMAS`，让"样本数在哪儿"和"是不是分块结构"共用同一个元组**；
`verify_p13d_drift.py` 里 `check_pair` 改成问 `SUPPORTED_SCHEMAS`。

改完做了验证：造两份同 shape、只差帧率的 `/5` 证据跑校验器，
D1/D2 **确实产出了 4 行结论**——不是"改了但没生效"。

---

## 六、五轮实测（4K60 / 8 s / 480 帧 / 同一 APK）

| 轮 | 帧 | 丢 | xrun | mode | accepted | span | mtd/p99 | mtd/max | pf/p99 | pf/max |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| r1 | 480 | 0 | 0 | decimated | 160 | 7.95 s | 2.125 | 5.506 | 2.125 | 16.930 |
| r2 | 480 | 0 | 0 | decimated | 160 | 7.95 s | 1.125 | 1.340 | 1.125 | 4.690 |
| r3 | 480 | 0 | 0 | decimated | 160 | 7.95 s | 0.375 | 1.358 | 1.125 | 3.257 |
| r4 | 480 | 0 | 0 | decimated | 160 | 7.95 s | 0.125 | 3.270 | 0.875 | 3.270 |
| r5 | 480 | 0 | 0 | decimated | 160 | 7.95 s | 0.625 | 3.414 | 1.625 | 4.528 |

五轮零丢帧、零 xrun、`mode` 全部 `decimated`、`accepted` 全部 160、`span` 全部 7.95 s。
**这些轮次没有走到降级分支**——它们是本卡的**回归基线**，不是降级的证据。

`trend.usable` 五轮全为 `false`（8 s < 30 s 统计门）。这与 G1 无关：
**结构门过、统计门不过**，正是本卡设计要区分的那两种事情。

---

## 七、诚实清单（七条）

1. **没有任何真机证据表明有流会缺 pts。** 本卡修的是"如果发生，证据会长成什么样"。
   没有复现，也没有为它改 `DecodedFrame` 的初值（见设计 §3.4 的选项 B）。

2. **`samples >= 2` 是结构门，不是统计门。** 2 个样本的 p99 仍然不可信，
   它只保证"这不是单点"。可信度归 `DriftTrend` 管。

3. **`span` 是序列窗口的首末差，不是全片跨度。** `DriftSeries` 容量 2048 点 / 20 Hz
   ≈ **102 秒**。超过之后窗口首末差恒定在 ~102 s，而 `accepted` 继续累计 ——
   `observed_hz = accepted / span` 会**偏高**，`G1` 的 `span > 0` 不受影响但这条公式受影响。
   这是 P13-D 起就有的问题，**本卡没修**，8 秒片源碰不到。

4. **`kPerFrame` 现在可达了，但本卡没有真机证据让它被走到。**
   要造它需要一个"媒体时间不推进但有逐帧数据"的片源，没造。
   区别在于：现在它的理由是准的（`decimated_unusable_reason`），不再是编的。

5. **层 2 不查 G 判据。** `card_checks` 只管"字段在不在、版本对不对"。
   "层 2 通过"不代表抽稀块够格——与 P13-F §五.5 同一类边界。

6. **`/3` `/4` 的旧证据仍按旧判据读。** 26 + 20 份不重写（重写等于伪造）。
   所以"G1 通过"这句话只覆盖 `p13g/` 那 10 份；旧证据里有没有 `samples = 1` 的块，
   本卡没有回溯检查——它们的 `accepted` 都是 160，事实上不存在这种块，但那是**读出来的**，
   不是判据保证的。

7. **全量构建仍有两处既有错误**，与本卡无关（`ffmpeg_adapter.cpp` 缺 `AVCodecContext`
   声明、`evidence_tests.cpp` 初始化列表转换）。两个文件 `git status` 干净，我没碰过。
   本卡相关测试 6 个目标全过。

---

## 八、下一步

1. **观察 102 秒的边界。** §七.3 那条：`observed_hz` 在超 102 秒的会话里会偏高。
   要修得让 `DriftSeries` 单独累计"首次接受时刻"与"最后接受时刻"，
   而不是从窗口首末推。这是个真缺陷，只是 8 秒片源永远碰不到。

2. **漂移量准仍卡在 7 ppm 的尺子上。** 从 P13-D 到 P13-G 一个字没动：
   0.25 ms 桶宽 / 8 s 片源，分辨率上限 ≈ 7 ppm。要谈 1 ppm 先换尺子。

3. **88 ms 那个尖峰仍未归因。** P13-F 十轮没复现，本卡五轮也没有
   （`mtd/max` 最大 5.506 ms）。它至今**没有被解释**，
   只是现在有了"下次发生时能定位"的工具（`worst`）和"下次发生时不会被当成正常数据"的门槛。

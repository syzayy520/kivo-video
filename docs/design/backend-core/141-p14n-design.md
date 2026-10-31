# 141 · P14-N 设计：把可测性接到 `sync` 报告

> 卡片：P14-N
> 前置：P14-L（`137-p14l-design.md`）引入 `MeasuredCounter` 并接到 `decode` 报告；
> P14-M（`139-p14m-design.md`）先把 drift 判据的覆盖面补齐
> 结论：`av_sync_report.cxx` 的 `over_budget_frames` 与 `lateness_ns` 各接一对
> `measured` / `reason`，键名与 `decode/18` 完全一致；`sync` 侧**只做这两处**
> schema：`kivo.p12c.sync/24` → `/25`

---

## 一、问题：同一轮播放，两份报告对同一个概念说出两种可信度

### 1.1 真机证据

`docs/evidence/p14l/` 的两份 `sync` 证据，同设备、同片源（1080p30）、只换排程档：

| 字段 | `cpu_paced` | `surface_at_time` | 读者会怎么读 |
|---|---|---|---|
| `pace.frames_decided` | 1800 | 1800 | 两档都跑了 1800 次决策 |
| `pace.presented` | 1800 | 1800 | 都播了 1800 帧 |
| `pace.dropped` | 0 | 0 | 一帧没丢 |
| `pace.lateness_ns.samples` | **1800** | **0** | |
| `pace.lateness_ns.p50` | **125000** | **0** | 右边读起来像"零延迟" |
| `pace.lateness_ns.p99` | **125000** | **0** | 同上 |
| `pace.lateness_ns.max` | **615260** | **0** | 同上 —— 而 0 是最完美的最大值 |
| `pace.over_budget_frames` | 0 | 0 | 都是"没有超预算帧" |

右列那四个 0 **不是测出来的**：`surface_at_time` 把上屏时刻交给合成器，
`note_presented` 从不被调用，`lateness_` 直方图一个样本都没进。
`over_budget_frames` 由 `count_abs_beyond()` 从那个空直方图数出来，因此必然是 0。

**同一件事，`decode` 报告已经会说实话了**（P14-L，`decode/18`）：

```json
"lateness_ns": { "samples": 0, "measured": false,
                 "reason": "no present-timestamp samples: …" }
"over_budget_frames_measured": false,
"over_budget_frames_reason": "no present-timestamp samples: …"
```

而 `sync` 报告（`sync/24`）里同一批字段**仍是裸数**。同一轮播放、同一个
`PaceSummary`，两份证据对"lateness 量到了没有"给出两种答案 —— 一份说
"没量到，因为……"，另一份把一个 0 摆在那里，让读者自己猜。

### 1.2 为什么这一格比 `decode` 侧更要紧

`sync` 报告是 G7（音视频同步）的**唯一**证据来源。G6 的判据 `over_budget_frames`
在两份报告里都有，但 G7 的判定几乎全部依赖 `sync`：

- 漂移趋势（`av_drift.trend`）只在 `sync` 里；
- `lateness_ns` 与 `schedule_lead_ns` 的对照只在 `sync` 里；
- 一份"两档都通过"的对照表，若其中一档的 `lateness` 是空读数，
  那张表就是拿"没量"和"量到 0"做比较。

### 1.3 为什么 P14-L 当时不能做

P14-L 的诚实清单第一条写着「`sync` 报告同一个缺陷没修」。当时的判断是
"成本低，下一张卡做"。**P14-M 改变了这件事的性质**：

`sync` 证据在 P14-M 之前**根本没有被判据看过**（`SCHEMA_ORDER` 停在 `/13`，
而报告已到 `/24`；总收口 `glob` 只扫 `p13*`）。在这种情况下往 `sync` 里加字段，
新字段会带着一整块没人验的区域通过总收口 —— 那正是 P13-K 存在的理由。

P14-M 修好之后（覆盖 54 → 177 份，认不出的新版本判红），这一格才有意义。

---

## 二、边界：只做两处，其余如实留白

`sync` 的 `pace` 块里"沉默的零"**不止两处**。按 P14-L 诚实清单点名的范围，
本卡只做它点名的两处，其余逐条写进诚实清单并说明为什么不在这张卡做。

| 字段 | 本卡做不做 | 理由 |
|---|---|---|
| `over_budget_frames` | ✅ 做 | P14-L 点名；G6 判据 |
| `lateness_ns`（`samples`/`p50`/`p99`/`max`） | ✅ 做 | P14-L 点名；G7 的核心输入 |
| `catch_up_events` / `drop_ratio` | — | **`sync` 报告根本不输出这两个字段**（只在 `decode` 里有）。本卡不新增字段 |
| `schedule_lead_ns` | ❌ 不做 | 它在 `surface_at_time` 档**有** 1800 个样本、在 `cpu_paced` 档是 0 —— 反向的沉默零。但那一档我们本来就不该用它（提交提前量在自定时档才有意义），加 `measured` 会把"用错尺子"变成"尺子没校准"，反而模糊。写进诚实清单 |
| `closure_gap_ns` / 四个 `span` / `first_lateness_ns` / `last_lateness_ns` | ❌ 不做 | 它们已有接线自证（`clock_supplied_frames` / `due_step_supplied` < `presented` 时整块作废），语义上不缺血。真正的问题是「自证字段本身也是 0」时怎么读，那是另一张卡 |
| `due_step_frames` | ❌ 不做 | 同上，`due_step_supplied` 已是它的自证 |

> 判据必须能抓住"只改一半"：Q5 就是钉这条 —— `over_budget_frames_measured`
> 与 `lateness_ns.measured` 必须**同为真或同为假**。它们是同一条派生链
> （`lateness_` → `count_abs_beyond`），若分家，说明有人各判了一次。

---

## 三、设计

### 3.1 键名：与 `decode/18` 逐字一致

不做"更优雅"的嵌套。同一个概念在两份报告里必须用同一个键名，否则读者会在
两套命名之间做翻译，而翻译就会出错。

| 位置 | 键 | 类型 |
|---|---|---|
| `pace` | `over_budget_frames_measured` | bool |
| `pace` | `over_budget_frames_reason` | string（量到时为 `""`） |
| `pace.lateness_ns` | `measured` | bool |
| `pace.lateness_ns` | `reason` | string |

`lateness_ns` 的 `measured` / `reason` 放在块**末尾**，与 `decode/18` 的字段顺序一致
（`samples` / `p50` / `p99` / `max` / `measured` / `reason`）。

### 3.2 值从哪来：不在报告层判

四个 `MeasuredCounter` 已经挂在 `PaceSummary` 上（P14-L 做的），
`PaceStats::summary()` 与 `FramePacer::summary()` 已经填好。报告层**只负责写出去**：

```cpp
const core::pace::MeasuredCounter& lateness_measured = pace.present_timestamp_measured;
const core::pace::MeasuredCounter& over_budget_measured = pace.over_budget_measured;
```

这是 P14-L 留下的分工，本卡不打破它：判在 `core::pace`（桌面可单测），
写在各报告层（平台相关）。若在报告层再判一次「`latency_samples > 0`」，
两条链迟早会在某个边界上分家（P14-M 的 D2 就是分组键漏了一项的同类错误）。

### 3.3 schema `24` → `25`

按既有规矩：**加字段就升版本**。升版本的一个直接后果是 P14-M 新加的
"认不出的新版本判红"会立刻生效 —— 这正好是它该做的事：`/25` 出现时，
任何还在用旧 `SCHEMA_LATEST` 的校验器都会报红，而不是静默跳过。

因此本卡的顺序是**先写判据再升版本**（见 §四），否则升完那一刻总收口会红着。

---

## 四、判据：Q 组（`verify_p13d_drift.py`）

六条，全部从 JSON 独立复算，不看 C++ 实现：

| 编号 | 判据 | 抓什么 |
|---|---|---|
| Q0 | `/25` 起必须有这两对字段 | 漏写 |
| Q1 | `measured=true` ⇒ `reason == ""` | 自相矛盾（量到了却带理由） |
| Q2 | `measured=false` ⇒ `reason` 非空且不等于那句证词 | "没量到但不说为什么" |
| Q3 | `measured=false` ⇒ 对应的值必须全为 0 | 没量到却报了非零（比沉默的零更糟） |
| Q4 | `lateness_ns.measured == (samples > 0)` | 可测性与样本数分家 |
| Q5 | `over_budget_frames_measured == lateness_ns.measured` | 同一条派生链被各判一次 |

Q3 有个容易写错的地方：`over_budget_frames` 与 `lateness_ns.p50/p99/max` 在
`measured=false` 时**本来就都是 0**（因为直方图是空的）。所以 Q3 在现有数据上
永远成立 —— 它抓的是**将来**有人让空直方图吐出非零（比如桶中心从 0 改成别的）。
为了证明 Q3 真的能红，变异自测里必须**手工构造**一份"measured=false 但值非零"。

Q4 与 Q5 是两条独立的病：Q4 抓"样本数改了但标志没跟上"，Q5 抓"两处各判一次"。
拿掉任一条，另一条都能被绕过。

---

## 五、验收

| 项 | 判据 |
|---|---|
| 桌面构建 | `kivo_p12c_core_pace` 编过 |
| 门禁测试 | `ctest -C Debug -R "kivo_p1[234]"` 25/25 |
| 旧证据 | `p14l/*-sync.json`（`/24`）**不被** Q 组误判（`/24` 没有这两对字段，Q0 只对 `/25` 起生效） |
| 新证据 | 真机两档，`/25`，`surface_at_time` 档两处 `measured=false`、`cpu_paced` 档 `true` |
| 总收口 | `verify_p12_all.py` 全绿，覆盖份数不降 |
| 变异 | Q1–Q5 各至少一条能红 |

**"旧证据不被误判"这一条不能靠豁免实现**：Q 组的门槛是 `schema >= /25`，
`/24` 及更早的文档不进 Q 组。这不是放水 —— 那些证据生成时字段还不存在，
拿新判据去判只会产出一批假红（P14-M 修的就是这类假红）。

---

## 六、不做

1. **不给 `schedule_lead_ns` 加 `measured`** —— 见 §二。
2. **不动 `closure_gap_ns` 与四个 `span`** —— 见 §二。
3. **不在报告层做任何判断** —— 值只能来自 `PaceSummary`。
4. **不给 `decode` 报告重复加字段** —— 它已经有了。
5. **不改 `MeasuredCounter` 的结构** —— P14-L 已定，本卡只用。

---

## 七、自然家族树

本卡**不新增文件**，只在既有家族里加一格。

```
backend/src/video/video_plane/backends/avsync/report/
└── av_sync_report.cxx        ← 改：pace 块两处 + schema 常量 + 历史注释

backend/include_private/video/video_plane/core/pace/
└── pace_stats.hpp            ← 不改：MeasuredCounter 已在 P14-L 挂好

tools/
├── verify_p13d_drift.py      ← 改：Q 组判据 + SCHEMA_LATEST 25
└── verify_p12_all.py         ← 改：schema 表登记 /25

docs/
├── design/backend-core/141-p14n-design.md
├── design/backend-core/142-p14n-status.md
└── evidence/p14n/            ← 新：真机两档
```

**一个文件一件事**：
`av_sync_report.cxx` 的职责是"把 `PaceSummary` 写成 JSON"，不是"判断可测性"——
所以本卡在它里面只加 `out <<`，一行判断都不加。判断住在 `pace_stats.cxx`。

# 145 · P14-P 设计：同一个直方图的两次出版，不能只有一次说精度

> 卡片：P14-P
> 前置：P14-O（`143-p14o-design.md`）在 §二 把「`pace.lateness_ns` 的范围」明确推迟，
> 理由是「那是另一条链：由 `PaceSummary` 直接出版，不经 `render_quantiles`」。
> 本卡就是那一项。
> 结论：`pace.lateness_ns` 接上 `p50` / `p99` 各三格 + `bucket_width_ns`；`sync/26` → `/27`；
> 新增 R0–R5，其中 **R5 把这一块与 `quantiles.per_frame` 的 11 个数逐字钉在一起**。

---

## 一、问题：门禁读的那一份，恰好是没有误差棒的那一份

### 1.1 现场

`docs/evidence/p14o/p14o-1080p30-sync.json`（`/26`，cpu_paced，1080p30，1800 帧）里，
同一个直方图 `PaceSummary::lateness` 被**出版了两遍**：

| 位置 | `p50` | `p50` 范围 | `p99` | `p99` 范围 | 谁读它 |
|---|---|---|---|---|---|
| `quantiles.per_frame` | 125 000 | **[15 417, 250 000)** | 125 000 | **[15 417, 250 000)** | N0–N13 判据 |
| `pace.lateness_ns` | 125 000 | **（没有）** | 125 000 | **（没有）** | `verify_p13ae_gate.py` 的 F3 |

右列不是"另一份测量"，是**同一批 1800 个点**的另一次印刷。`av_sync_report.cxx` 里两处
调用的是同一个函数族：

```cpp
render_quantiles(out, pace.lateness, ...);                    // → quantiles.per_frame
out << pace.lateness.percentile_ns(0.99);                     // → pace.lateness_ns
```

于是 `125 000` 这个数在一份 JSON 里有两种身份：在上面它是"桶中心，真值可能落在
15 µs–250 µs 之间的任何地方"，在下面它长得像一个精确到纳秒的观测。

### 1.2 为什么这件事要紧

`tools/verify_p13ae_gate.py:79,156`：

```python
p99 = int(pace["lateness_ns"]["p99"])
if frame_ns > 0 and p99 > frame_ns:
    raise Failure(f"F3 lateness p99 ... 超过一帧")
```

F3 的阈值是**一帧**，而它手上的数分辨力是**一个桶（250 µs）**。30 fps 下一帧是
33 333 333 ns，桶宽是它的 0.75% —— 这个余量眼下够用，所以 F3 至今没被这条坑到。
但"够用"不是"看得见"：判据在阈值附近**没有能力说出自己没能力判**。

更要紧的是另一半：读者拿到 `pace.lateness_ns` 时，**没有任何字段说明 `125 000` 是个桶中心**。
他要么去翻两百行以上的 `quantiles.per_frame` 自己发现两处是同一个数，要么去读
`histogram.buckets` 数一遍 —— 而判据的全部意义就是不要求他数。

### 1.3 这与 P14-N 是同一课，只换了层次

P14-N 修的是**可测性**：`lateness_ns.samples = 0` 时那三个 0 不是"零延迟"，是空直方图的默认值，
所以补了 `measured` / `reason`。

本卡修的是**分辨力**：`samples = 1800` 时那三个数确实测到了，但测到的是"落在哪个桶"，
不是"等于多少"。P14-N 让"没量到"变显式，本卡让"量到多粗"变显式 —— 同一块、
同一批字段、同一句"别让 0 冒充观测"的第二个层次。

P14-O 只把这件事做到了 `quantiles` 那一侧。那时它写清了理由（见 §二引文），
本卡把那句话落地。

---

## 二、边界

| 项 | 本卡做不做 | 理由 |
|---|---|---|
| `pace.lateness_ns.p50/p99` 各三格 + `bucket_width_ns` | ✅ 做 | 本卡存在的理由 |
| R5：与 `quantiles.per_frame` 逐字相等 | ✅ 做 | 本卡唯一的硬判据，见 §四.2 |
| 同一件事补到 `pace.schedule_lead_ns` | ❌ 不做 | 它的第二个数不是分位数而是 **`spread = p90 − p10`**（两个桶中心相减，误差可达两个桶宽）。给差值出版误差是另一道题，不是"再印三格"。单列一卡 |
| 给 `p95` 出版范围 | ❌ 不做 | 没有任何判据消费它（P14-O 已定，本卡不翻案） |
| 细化桶宽 | ❌ 不做 | 改桶宽等于改全部历史证据的可比性。本卡只让精度**可见** |
| 让 `lateness_ns` 带上 `histogram` | ❌ 不做 | 同一份 JSON 里出现两份逐桶分布，就是 §1.1 那个错换个方向再犯一次。**用 R5 把两份绑在一起，而不是复制数据** |
| 删掉 `pace.lateness_ns` 只留 `quantiles.per_frame` | ❌ 不做 | 见 §3.1：`quantiles` 在 `drift_measurable == false` 时是空对象，而这个块必须恒在 |
| 改 F3 的判定 | ❌ 不做 | F3 是门禁，改它要单独的卡与单独的现场。本卡只让它的输入带上分辨力 |

### 2.1 为什么不能干脆删掉这个块

`av_sync_report.cxx` 里 `quantiles` 是**有条件**的：

```cpp
if (!drift_measurable) { out << "},\n"; }   // 空对象
```

`surface_at_time` 档正是这种情况（`p14l-attime-1080p30-sync.json`：`quantiles = {}`）。
而 `pace.lateness_ns` **恒在**，它带着 `samples = 0` / `measured = false` / 理由，
是那一档唯一能说明"lateness 一条都没采到"的地方。

删掉它，等于在"最需要解释为什么什么都没量到"的那一档上把解释删掉。
所以只能补全，不能合并。

---

## 三、设计

### 3.1 七格，位置紧贴被解释的数

```
"lateness_ns": {
  "samples": 1800,
  "p50": 125000,
  "p50_range_low_ns": 15417,
  "p50_range_high_ns": 250000,
  "p50_bucket_samples": 1794,
  "bucket_width_ns": 250000,
  "p99": 125000,
  "p99_range_low_ns": 15417,
  "p99_range_high_ns": 250000,
  "p99_bucket_samples": 1794,
  "max": 948718,
  "measured": true,
  "reason": ""
},
```

键名与 `quantiles.<口径>` 那六格**逐字对齐**（只差块名）。三格紧贴各自解释的那个数，
`bucket_width_ns` 跟在 `p50` 那组之后 —— 与 `render_quantiles` 的排布完全一致：
同一个概念在两处长得不一样，读者就要在脑子里做翻译，而翻译会出错。

### 3.2 报告层：调同一个函数，不复制这六行

`render_range_cells(out, pad, prefix, histogram, q)` 是 P14-O 从 `render_quantiles` 里
抽出来的（职责只有一句：印 `<prefix>_range_low_ns` / `_high_ns` / `_bucket_samples`）。
本卡在 `lateness_ns` 块里调它两次，`prefix` 取 `"p50"` / `"p99"`。

**不复制那六行**的理由与 P14-O 写下的完全相同：一次复制错前缀，
`p99_range_low_ns` 里装的就会是 `p50` 的范围，而 JSON 里它看起来完全正常。

它仍然**只印不算** —— 范围由 `core::pace::SignedHistogram::percentile_range(q)` 给。
本文件只在 Android 侧编译，写在这里的钳制逻辑没有桌面单测兜底。

### 3.3 核心层一个字不改

`percentile_range(q)` / `percentile_bucket(q)` / `kBucketWidthNs` 都已在 `SignedHistogram` 上。
本卡**没有新增任何算法**，只是给第二个调用点接上已有的接口。

这条要写进状态卡：连续两张卡（P14-O、P14-P）都是"发现已有接口只有一个调用点"，
不是"实现了新东西"。这种卡的价值在判据，不在代码。

### 3.4 schema `26` → `27`

加字段就升版本。顺序仍然是**先改判据 → 再升版本 → 最后取证**。
升版本后 P14-M 那条"认不出的高版本判红"立刻生效：任何还停在 `SCHEMA_LATEST = 26`
的校验器都会报红，而不是静默跳过。

---

## 四、判据

### 4.1 R0–R4：自洽（`verify_p13d_drift.py`，门槛 `schema >= /27`）

| 编号 | 判据 | 抓什么 |
|---|---|---|
| R0 | 块在，且带七格 | 报告层只改了 `quantiles` 那条路 |
| R1 | `bucket_width_ns == 250000` | 在 Python 侧独立写死，不从证据读 —— 读出来再和自己比只叫复述 |
| R2 | `p50` / `p99` 各自落在自己声明的 `[low, high]` 内 | 范围不含它要解释的数 |
| R3 | `0 ≤ bucket_samples ≤ samples`（两处） | 落在一个桶里的样本多于全部样本 |
| R4 | `measured == false ⇒ 七格全 0` | 没量到却印出非零（Q3 只守了 `p50/p99/max` 三个，本卡的新四格在它视野外） |

R4 的延伸不是洁癖：`/25` 的 Q3 判的是 `samples / p50 / p99 / max` 四格。
本卡新增的四格（两个 `range_low`/`range_high`、两个 `bucket_samples`）
**不在 Q3 的清单里**，不补这一条就是一个新的、无人看守的空洞。

### 4.2 R5：与孪生块逐字相等 —— 本卡的核心

`quantiles.per_frame` 与 `pace.lateness_ns` 来自**同一个** `PaceSummary::lateness`。
所以下面 11 个数必须逐位相等：

```
samples, p50, p50_range_low_ns, p50_range_high_ns, p50_bucket_samples,
bucket_width_ns, p99, p99_range_low_ns, p99_range_high_ns, p99_bucket_samples, max
```

这一条同时做两件事：

1. **抓分家。** 两处出版同一次测量，任何一处被单独改动都会立刻红。
   这是 P14-N 的 Q5（`over_budget_measured` 与 `lateness.measured` 必须同值）
   在"数值层"的同一招：同源的量，判据不许它们给出两个答案。
2. **把复算性借过来。** `lateness_ns` 没有 `histogram`，所以它的三格无法按分布复算
   （N12/N13 那套在这里跑不了）。但 `quantiles.per_frame` **有**分布，而它的三格
   已经被 N12/N13 按 `histogram` 逐位复算过。R5 把这条链传过来 ——
   `lateness_ns` 因此**间接可复算**，而代价是零字节的重复数据。

孪生块缺席时（`drift_measurable == false`，`quantiles` 是空对象）R5 **判"不可判"**，
理由写在输出里。这沿用 N12/N13 的既有写法（绿 + 明说"没有桶可复算"），
但措辞必须出现"不可判"，不能写成一个干净的"通过"。

### 4.3 判据放在哪儿

Q 组之后、`drift = doc.get("av_drift", {})` 之前。理由：R 组**完全不读 `av_drift`**，
而 `check_file()` 在 `sampling` 缺失、`mode == not_measured` 两处会提前 `return`。
P14-N 的 Q 组第一次就是写在所有 `return` 之后，于是六条判据一条都没跑到 ——
本卡不接受同一个错犯第二次，所以位置按"它需要什么"来定，不按"读起来顺"来定。

---

## 五、验收

| 项 | 判据 |
|---|---|
| 桌面构建 | 无核心层改动，`kivo_p12c_core_pace` 应零变化 |
| 单测 | `quantile_tests` 不新增（没有新算法）；门禁 25/25 不许降 |
| 旧证据 | `/26` 及更早**不被** R0–R5 误判（门槛 `>= /27`） |
| 新证据 | 真机 1080p30 两档（cpu_paced / surface_at_time）：前者 `measured=true` 且 R5 相等，后者七格全 0 |
| 总收口 | `verify_p12_all.py` 全绿，覆盖份数不降 |
| 变异 | R0–R5 各至少一条，全部按预期 |

**预言（取证后必须核对，错了不改回设计卡）**：

| 量 | 预言值 | 依据 |
|---|---|---|
| cpu_paced 的 `lateness_ns` 与 `quantiles.per_frame` | **11 个数逐字相等** | 同一个 `PaceSummary::lateness`，两次印刷 |
| cpu_paced 的 `p50_range` == `p99_range` | **相等（且都是 [15417, 250000) 量级）** | 两个分位都落在桶 512（P14-O 已复算） |
| surface_at_time 的七格 | **全 0，`measured = false`** | 空直方图 + `present_timestamp_measured` 为 false |

第二条同时是本卡的**已知盲区**：真机上 `p50` 与 `p99` 同桶时，"把 `p50` 的三格抄给 `p99`"
与"按 q=0.99 正确计算"**得到的结果本来相同**。所以在真机证据上 R5 无法区分这两种写法 ——
这一条只能靠变异自测（人为抬尾巴让两者分家）证伪。P14-O 已经踩过并记下了同一课。

---

## 六、不做

1. **不给 `schedule_lead_ns` 出版误差。** 它的第二个数是差值，不是分位数。见 §二。
2. **不复制 `histogram` 进 `lateness_ns`。** 用 R5 绑定，不复制数据。
3. **不动 Q0–Q5、N0–N13。** 已经绿的东西不碰；R4 是 Q3 的**延伸**，不是替代。
4. **不改 F3。** 门禁判定要单独一张卡、单独一个现场。
5. **不细化桶宽。** 那会让全部历史证据失去可比性。

---

## 七、自然家族树

本卡**不新增源文件**（除脚本与文档），只在既有家族里各加一格。

```
backend/include_private/video/video_plane/core/pace/
└── pace_histogram.hpp            ← 不改：percentile_range / percentile_bucket / kBucketWidthNs 都已存在

backend/src/video/video_plane/backends/avsync/report/
└── av_sync_report.cxx            ← 改：lateness_ns 七格（调 render_range_cells 两次）+ schema /27

backend/tests/                         ← 不改：没有新算法，没有新单测
tools/
├── verify_p13d_drift.py           ← 改：R0–R5 + SCHEMA_WITH_LATENESS_RANGE + SCHEMA_LATEST 27
├── verify_p12_all.py              ← 改：两张表登记 /27
└── run_p14p_lateness_range.sh     ← 新：两档各一轮（cpu_paced / surface_at_time）

docs/
├── design/backend-core/145-p14p-design.md
├── design/backend-core/146-p14p-status.md
└── evidence/p14p/                 ← 新：真机两轮
```

**一个文件一件事**：
`av_sync_report.cxx` 的职责是"把统计量写成 JSON" —— 它现在把同一个直方图印两遍，
两遍的**内容必须一致**，因为差异只可能来自写法不同，不可能来自数据不同；
"这两遍是不是一致"由 `verify_p13d_drift.py` 的 R5 判，**报告层不自己判自己**。
一处印刷、一处判定，没有一处同时知道两件事。

# 137 · P14-L 设计：派生计数不许「沉默的零」

> 卡片：P14-L（P14 系列收尾）
> 前置：P14-K（`135-p14k-design.md`）给 `lateness_ns` 加了 `measured`，但同一类病还剩三格
> 结论：引入 `MeasuredCounter`，让 `over_budget_frames` / `catch_up_*` / `drop_ratio`
> 各自说出「量到没有、没量到是为什么」；判据 P9–P11 守住派生链
> schema：`kivo.p12c.decode/17` → `/18`

---

## 一、问题：一个必须一次治完的病

### 1.1 P14-K 只修了一半

真机证据 `docs/evidence/p14k/p14k-4k60.json`（`surface_at_time`，4K60 / 480 帧）：

| 字段 | 值 | 真正含义 |
|---|---|---|
| `pace.lateness_ns.samples` | **0** | 一个样本都没有 |
| `pace.lateness_ns.measured` | **false** | P14-K 补的，正确 |
| `pace.over_budget_frames` | **0** | **从上面那个空直方图数出来的** |
| `pace.catch_up_events` | 0 | 真的 0（这一档提前 3 帧，永不迟到） |
| `pace.drop_ratio` | 0.0 | 真的 0（480 帧全播，一帧没丢） |

第一行与第三行是同一条派生链：

```
lateness_（空）──► count_abs_beyond(frame_duration) ──► over_budget_frames = 0
```

`over_budget_frames` 是 G6（帧节奏）的判据。所以：

> **G6 在整个 `surface_at_time` 档上读着一个永远为 0 的数，一直"通过"着一次从未发生的检查。**

P14-K 修了上游（`lateness.measured`），下游那格还在印 0。而这正是 P14-K 状态卡
§三.2 自己写下的那句：「要加就一起加 `measured` 标志，而不是打一个补丁留三个。」

### 1.2 为什么不能只加一个布尔

若只给 `over_budget_frames` 加 `measured`，它必然与 `lateness.measured` 判同一个条件。
两处各判一次，迟早会在某个边界上分家 —— 而分家之后，证据会**同时**说
「lateness 没量到」和「超预算帧量到了」。读者只会看见后者。

分家的病这个项目栽过：`158` 的 `observed_hz`（P13-H）就是校验器与实现各算一遍同一个公式。

所以本卡的核心不是「加字段」，是**让派生关系成为一条链、只有一个判断点**。

### 1.3 三格的「0」各自是什么意思

| 字段 | 为 0 时可能是 | 判别条件 |
|---|---|---|
| `over_budget_frames` | ① 真的一帧都没超 ② **上游没样本** | `lateness.samples > 0` |
| `catch_up_events` / `catch_up_frames` | ① 真的没追赶 ② **一帧都没决策过** | `frames_decided > 0` |
| `drop_ratio` | ① 真的零丢帧 ② **分母为 0**（无定义） | `presented + dropped > 0` |

三格三种条件，不能共用一个布尔。但 `over_budget` 与 `lateness` **必须共用同一个**。

---

## 二、设计

### 2.1 `MeasuredCounter`

```cpp
struct MeasuredCounter {
    bool measured = false;
    const char* reason = kUnspecifiedReason;   // 默认就是那句证词
};
```

两个刻意的设计：

1. **`reason` 默认值是「没量到但没说为什么」**。任何一条链忘记赋值（新加派生量、
   忘了在 `summary()` 里填），出来的就是 `{false, "unmeasured without a recorded reason"}` ——
   判据 P11 专抓这个组合。留空字符串或默认 `{true, ""}` 都会静默通过。
2. **`reason` 是 `const char*` 不是 `std::string`**：理由全是静态字面量，而 `PaceSummary`
   每秒被整块复制数次。与 `DriftTrend::unusable_reason` 同一取舍。

`measured_ok()` / `measured_missing(reason)` 是仅有的两个构造入口，
所以 `measured == true` 时 `reason` 必然是空的 —— 「量到了但附了条理由」在结构上不可能出现。

### 2.2 三格的理由，两句共享

```
kNoPresentTimestampReason  lateness 与 over_budget_frames **共用**
kNoDecisionsReason         catch_up_events / catch_up_frames **共用**
kNoDenominatorReason       drop_ratio
```

共享不是省字，是**防止两处字面量被改成两句略有出入的话** —— 那时读者会
以为这是两个不同原因。`over_budget` 与 `lateness` 共用同一句，正是"它们是同一条链"的表达。

### 2.3 判断放在「知道的那个层」

| 格 | 判断位置 | 为什么 |
|---|---|---|
| `present_timestamp_measured` | `PaceStats::summary()` | 它拥有 `latency_samples_` |
| `over_budget_measured` | **复制上者** | 派生量，不重判 |
| `drop_ratio_measured` | `PaceStats::summary()` | 它拥有 `presented_` / `dropped_` |
| `catch_up_measured` | `FramePacer::summary()` | **只有它知道 `frames_decided_`** |

最后一行是这张卡最容易做错的地方：`PaceStats::summary()` 里的 `frames_decided`
恒为 0（那个字段由 `FramePacer` 事后填），在那里判 `frames_decided > 0`
**永远得到「没决策过」**。做法与 P13-M 的 `note_anchor_reset()` 一致：问那个确实知道的地方。

渲染层（`decode_stats_renderer.cxx`）只负责写出去，不判任何条件。

### 2.4 JSON 形态：兄弟字段，不改既有整数

```json
"drop_ratio": 0.0,
"drop_ratio_measured": true,
"drop_ratio_reason": "",
"catch_up_events": 0,
"catch_up_frames": 0,
"catch_up_measured": true,
"catch_up_reason": "",
"over_budget_frames": 0,
"over_budget_frames_measured": false,
"over_budget_frames_reason": "no present-timestamp samples: ..."
```

不把整数改成对象：`over_budget_frames` 已经是被下游读的整数，改成对象是破坏性变更，
而这一卡的目的是**让 0 读得懂**，不是换一套读取路径。

---

## 三、判据

| 编号 | 内容 |
|---|---|
| P9 | `over_budget_frames_measured == lateness_ns.measured`；且 `false` 时 `over_budget_frames == 0` |
| P10 | `catch_up_measured == (frames_decided > 0)`；`drop_ratio_measured == (presented + dropped > 0)` |
| P11 | 每格 `measured=true` 不许带理由；`false` 必须有理由，且不能是 `kUnspecifiedReason` |

P11 的第二半是这一组里最便宜的守护：它抓的是「忘了赋值」，而那种错
不会让任何数变成 NaN、不会让任何测试变红 —— 只会让证据安静地少一句解释。

---

## 四、自然家族树

```
backend/include_private/video/video_plane/core/pace/
├── pace_measurability.hpp        新：MeasuredCounter + 三个理由常量
├── pace_stats.hpp                改：PaceSummary 增 4 个 MeasuredCounter
backend/src/video/video_plane/core/pace/
├── pace_measurability.cxx        新：measured_missing 的空理由守卫
├── pace_stats.cxx                改：填 3 格（over_budget 复制上游）
├── frame_pacer.cxx               改：填 catch_up（只有它知道 frames_decided）
backend/include_private/video/video_plane/backends/mediacodec/session/
└── decode_pace_stats.hpp         改：4 格原样转发
backend/src/video/video_plane/backends/mediacodec/session/
├── decode_pace_report.cxx        改：转发，不重判
└── decode_stats_renderer.cxx     改：sync /17 → /18，写出 _measured / _reason
tools/
├── verify_p14i_gap.py            改：+P9/P10/P11
└── verify_p12_all.py             改：登记 decode/18
```

**为什么 `MeasuredCounter` 独立成单元**：它是「证据语义」而不是节拍器内部状态，
`PaceSummary`、`DecodePaceStats`、渲染层三处都要用它，而理由字符串必须只有一份。
放进 `pace_stats.hpp` 会让那两处 include 一个它们并不需要的聚合器头。

---

## 五、不做什么

1. **不动 `sync` 报告（`kivo.p12c.sync/24`）。** 它同样有 `over_budget_frames`
   与 `lateness_ns`（无 `measured`），缺陷一模一样。但 P14-K 状态卡点名的范围只有
   `verify_p14i_gap`（decode 侧）；一张卡动两份 schema 会让两侧判据的版本门槛
   同时抬升，回滚成本翻倍。**如实写进状态卡的诚实清单，作为下一张卡的入口。**
2. **不改 `lateness` 的值本身。** 这一卡只让「有没有量到」可读，不改变任何测量方式。
3. **不解决 G6/G7 在默认档未验证。** 真要测到上屏时刻，得改呈现路径
   （合成器回调 / `onFrameAvailable`），那是独立的一项，不是加字段能做的。
   本卡的产出是**把"未验证"这件事变得不可能被误读成"已验证"**。
4. **不给 `interval_ns` 加 `measured`。** 它在两档下都有样本，只是含义随档位变
   （`surface_at_time` 下测的是提交间隔，不是显示间隔）—— 那是 P14-K §二 已指出的
   「同名字段跨档含义不同」，属于另一类问题。

---

## 六、验收

| 编号 | 内容 |
|---|---|
| P9–P11 | 见 §三 |
| 正例 | 真机两份（`surface_at_time` / `cpu_paced`）全绿 |
| 变异 | 五个变异各自被对应对据打红（分家 / 非零 / 分母 / 证词 / 自相矛盾） |
| 门禁 | `ctest -C Debug -R "kivo_p1[234]"` 25/25 |

真机验收看两件事：

- `surface_at_time`：`over_budget_frames_measured == false` 且理由非空 —— **这是本卡的存在理由**；
- `cpu_paced`：`over_budget_frames_measured == true` —— 同一台设备、同一片源，
  只换排程档，可测性就该翻转。两份证据并排放，读者一眼看出「默认档的 0 不是好消息」。

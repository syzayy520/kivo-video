# P14-V · 门限的量纲：45 ms 在不同长度的会话里是不同的事

- 卡号：P14-V
- schema：`kivo.p12c.sync/32 → /33`
- 判据：`verify_p12c_sync.py` V0–V4
- 上游：P13-S / P13-AC（门本身）、P14-S（速率）、P14-T（领先量）
- 状态：设计中 → 实现

---

## 1. 起因

`pace_policy.hpp` 里那一行：

```cpp
std::int64_t drift_limit_ns = 45'000'000;
```

注释写着「它就是 `kAudibleThresholdNs`（人耳能听出的音画偏差），与 P13-L 的额度判读同一把尺子」。

这句话在**偏移量**的意义上是对的：两根声道错开 45 ms 以上，人耳能听出来。
但 `drift_limit_ns` 在这套代码里守的不是偏移量，是**累积漂移**：

```
cum_drift += (clock_delta − wall_delta)        // clock_position_tracker.cxx
```

而累积漂移 = 速率误差 × 窗口时长。于是同一个 45 ms，在不同长度的会话里等价于完全不同的速率容差。

## 2. 实证（224 份证据，2026-10-31 普查）

| 窗口 | 份数 | 门限等价速率 | 读作 |
|---|---|---|---|
| 6.3 s | 1 | 7 138 ppm（0.71%） | 很宽松 —— 速率偏 0.7% 才撞门 |
| 8.0 s | 9 | 5 648 ppm（0.56%） | 宽松 |
| 60 s | 23 | 750 ppm（0.075%） | 收紧 |
| 130 s | 10 | 346 ppm | 更紧 |
| 300 s | 6 | 150 ppm | 接近晶振 |
| 1800 s | 8 | **25 ppm**（0.0025%） | **晶振精度** |

最宽与最窄相差 **285 倍**。

25 ppm 是什么量级：普通石英晶振的初始精度约 ±20–50 ppm，音频设备与系统单调钟之间的速率差通常就在这个区间。
**也就是说：播 30 分钟，一个完全正常的时钟就会撞门**，而门会判「漂移过大」并复位。

这不代表门限错了 —— 门守的是「累积偏差不超过人耳阈值」，而 25 ppm × 1800 s = 45 ms，逻辑自洽。
错的是**这句话从来没有被算出来过**。

## 3. 报告里的现状：两格数据并排，从不换算

同一份证据文件里：

```json
"drift_gate": { "enabled": true, "limit_ns": 45000000, "band_ns": 5000000 },
"pace":       { "elapsed_span_ns": 1799933677769, ... }
```

`limit_ns` 恒定 45 000 000 出现在每一份里（65 份全部相同），
`elapsed_span_ns` 从 6.3 s 到 1 800 s 摆 285 倍。
两格相邻，却从不互相比。

结果：读证据的人在 1800 s 那份上看到「门限 45 ms」，会觉得门很宽松；
实际上它已经收紧到 25 ppm —— 比 8 s 那份严 226 倍。

这是 P14-S（`video_pts_span_ns` 名不副实）和 P14-T（`av_drift` 参考系错）同一类的病：
**同一个数在不同上下文里是不同的事，而报告没说。**

## 4. 改法

**一个阈值都不动。** `drift_limit_ns` 保持 45 ms —— 动它就是调参，`pace_policy.hpp` 第 11 行明确写了「调参必须另开卡并给出对照数据」。这一卡只做出版。

新增 `core/pace/pace_gate_scale.{hpp,cxx}`，三个纯函数：

```cpp
/// 45 ms 在 `span_ns` 这么长的会话里，等价于多少 ppm 的速率误差。
/// 定义式：ppm = limit_ns × 1e6 / span_ns。不带测量含义。
GateScale scale(std::int64_t limit_ns, std::int64_t span_ns) noexcept;
```

`GateScale` 出版三格：

| 字段 | 含义 | 可测性 |
|---|---|---|
| `limit_ppm_equivalent` | 门限等价速率（ppm，double） | 需要 `span_ns > 0` |
| `limit_reach_s` | 以 `media_rate` 的实测偏差多久撞门（秒） | 需要 `media_rate_measured` |
| `reach_measured` | 上一格是不是算出来的 | 自证 |

`limit_reach_s = limit_ns / (|media_rate − 1| × 1e9)`。
`media_rate` 不可测（`media_rate_measured = false`）时不给 `limit_reach_s`，给理由。

**为什么 `reach` 要单独一格**：`ppm` 是「门限对应多严」，`reach` 是「这次会话实际会用多久撞到」。
8 s 窗口上 ppm = 5 648 但 `media_rate ≈ 1`，`reach` 可能是无穷（整段都撞不到）；
1800 s 窗口上 ppm = 25，`reach` 可能只有 300 s。两个数回答不同的问题。

### 家族树

```
core/pace/
├── pace_policy.hpp          阈值定义（本卡不动）
├── pace_stats.hpp/.cxx      记账（喂 scale）
├── pace_gate_scale.hpp/.cxx ← 新增：换算，纯函数，零状态
│     只依赖 <cstdint>/<optional>，不依赖 PaceStats/PacePolicy
└── pace_lead_trace / pace_wall_offset    （P14-U，与本卡无耦合）
```

`pace_gate_scale` 是**叶子**：别人 include 它，它不 include 任何人。
这样它可以被报告层直接调用，也可以被单测独立构造。

## 5. 判据

| 编号 | 断言 |
|---|---|
| V0 | `/33` 起 `drift_gate` 必须带 `limit_ppm_equivalent`；缺则红（防止又漏接） |
| V1 | `limit_ppm_equivalent` 逐位等于 `limit_ns × 1e6 / span_ns`（容差 1e-6 相对，定义式） |
| V2 | 有 `elapsed_span_ns > 0` 却 `limit_ppm_equivalent == 0` → 红（**零不当"未知"用**） |
| V3 | `reach_measured = true` 时，`limit_reach_s` 必须满足 `limit_reach_s × |rate−1| × 1e9 ≈ limit_ns`（反向解回门限） |
| V4 | `reach_measured = false` 时必须给 `limit_reach_reason` 非空 |

V2 是这一卡的**主要防线**：`span_ns == 0` 时函数返回「不可算」，
如果实现里图省事返回 0.0，那个 0 会被读成「门限等价于 0 ppm」，即「门严到不可用」——
正好是最容易被误读的那种假观测。

### 长会话的断言（不进判据，进分析器）

`tools/audit_gate_scale.py` 对 65 份门相关证据算等价速率并分组，输出：

- 每个窗口长度对应的 ppm
- 「正常晶振（25 ppm）能撑多久」vs「实际撑了多久」
- 反向判据：若某份 1800 s 的证据算出 > 100 ppm，说明它的 `span_ns` 或 `limit_ns` 错了

**为什么不进判据**：25 ppm 是事实，不是缺陷。把它写成判据（「必须 > 200 ppm」）会
把 8 份正确的长会话证据判红。判据只管一致性，事实交给分析器。

## 6. 预言（先写下来，测出来对不上就是设计错了）

1. `/33` 的第一份真机证据里，8 s 窗口的 `limit_ppm_equivalent` ≈ 5 600 ppm，
   而 `limit_reach_s` 会是 `null`（`media_rate ≈ 1`，分母为 0）。
2. 1800 s 那份老证据（若能重跑）`limit_ppm_equivalent` ≈ 25 ppm。
3. 注入 `kivo.clock_rate_ppm 5000` 的一轮：`limit_ppm_equivalent` 仍是 5 648，
   但 `limit_reach_s` ≈ 9 s —— 门会在第 9 秒左右撞到，比整段 8 s 还长，所以整段撞不到。
   这条**证伪了也很有价值**：说明 8 s 窗口根本测不出 5000 ppm 的速率误差。

第 3 条是这一卡可能带来的最大发现：**8 s 片源上的速率判据，量程不够**。

---

## 7. 不做的事

1. **不动 `drift_limit_ns`**。45 ms 是听感阈值，改它要另开卡给对照数据。
2. **不改 `drift_band_ns`**。迟滞带是 P13-AC 专门加的，与本卡无关。
3. **不把 ppm 当「合格线」**。这一卡出版的是「门限对应多严」，不是「这次合不合格」。
4. **不在 `pace_stats` 里缓存**。换算在出版时算一次，记账路径零成本。
5. **不给 `/32` 及更早版本补字段**。老证据就让它缺，
   缺是历史事实，补是伪造。

## 8. 与既有判据的关系

- **P14-S 的 ±2%（`KNOWN_OUT_OF_BAND`）**：那是「诊断带」，判测量是否结构性偏离。
  与门的 45 ms 是两把尺子，本卡把两者的换算关系写清，不合并。
- **P14-T 的 T3（门开着不许跑过去）**：用 `clock_lead_ns` 与 `limit_ns` 比。
  本卡补上「`limit_ns` 在这次会话里等价于多少 ppm」，让 T3 红的时候能看到严重程度。
- **P14-U 的 `vs_wall`**：与本卡正交。U 换参考系，V 换量纲。

三者合起来才能回答「这一次播放，音画到底差多少、门管到哪」。

---

## 9. 真机结果（2026-10-31，Redmi K40 / b6e950ee / 4K60 8 s / 门关）

三组，只改"钟被注入了什么"。

| 组 | `limit_ppm_equivalent` | `limit_reach_s` | `av_drift.p50` | `vs_wall.last` |
|---|---|---|---|---|
| 阶跃 +400 ms | 5 648.366 772 | **0.853 202** | 125 000 ns | **−399 961 544 ns** |
| 速率 +5000 ppm | 5 648.245 832 | **12.072 237** | 125 000 ns | −29 629 253 ns |
| 不注入 | 5 648.400 153 | 12 396.7 | 125 000 ns | 44 080 ns |

**预言 1 命中**（5 648 ppm），**预言 2 命中**（普查里 1 800 s 的 8 份全是 25 ppm），
**预言 3 命中且更强**：速率组 `reach = 12.07 s` > 整段 8 s，所以

> **8 s 片源上跑 5 000 ppm 的速率误差，门从头到尾一次都不会响。**

这就解释了为什么 P13-S 当时"8 秒累积 118.79 ms（1.49%），`jump_rebases` 全程 0"
—— 1.49% 在 8 s 上只攒到 118 ms？不，`drift_limit_ns` 是 45 ms，118 ms 早该撞了。
差别在门开没开：那一轮 `drift_gate.enabled` 是 false。**门关着的轮次不该被读成门的判断。**

### 9.1 阶跃组：同一份证据，两个答案

`p14u-step-400ms-sync.json` 里：

```
av_drift.quantiles.media_time_decimated.p50 = 125000        ← 125 µs，"完美"
av_drift.vs_wall.last_ns                    = -399961544    ← −400 ms
pace.clock_lead_ns                          = 400022450     ← 钟比墙多走 400 ms
drift_gate.limit_reach_s                    = 0.853202      ← 0.85 s 就会撞门
```

四句话出自同一批 449 帧。第一句是 P14-U 之前的全部答案，后三句是 P14-T/U/V 加起来才有的。
**这就是"45 ms 门限 + 相对钟的漂移"这套判据在钟撒谎时的全部盲区。**

### 9.2 实现中被迫改的两处（都不是设计时想到的）

**① `media_rate` 的出版精度不够复算。**

真机上 `p14u-control` 印出 `media_rate = 0.999996`（6 位小数）与
`limit_reach_s = 12 578.8 s`。用印出来的数反算 reach 得 11 250 s —— 差 12%。
因为 `reach = limit / |rate − 1|` 对偏差是**倒数**关系，6 位小数的舍入被放大。

修法不是放宽判据，是**让证据能被自己复算**：`media_rate` 与 `limit_reach_s`
一起印到 9 位，并且**先量化再算下游**（`published_rate()`）。
于是 V3 的容差能收到 `limit_ns × 1e-8`（0.45 ns）。

**② U4 的「2 000 ppm 上限」在注入轮次上不成立。**

`u14-ramp-5000ppm` 算出 3 742 ppm，被判成"超过物理上限却说是速率"。
但这一轮**就是人为注入 5 000 ppm 的**：注入只覆盖第 2 s 之后那 6 s，
而 `rate_ppm` 摊在整段 8 s 上，整段平均必然小于注入瞬时值。两个数都对。

修法：U4 的上限在注入轮次换成「注入值 × 1.05」（注入给的是一个天花板）。

### 9.3 顺带照出的两个既有缺陷

**③ `av_sync_report.cxx` 的 `av_drift` 块少一个逗号。**

P14-U 插入 `vs_wall` 时，`quantiles`/`trend` 闭合处的 `},` 变成了 `}`。
后果：三份真机证据**全都不是合法 JSON**，判据一份都读不进来。

而 P14-U 的判据变异自测当时是 9/9 全绿 —— 因为它跑在手工构造的合成 JSON 上，
那份 JSON 的语法本来就是对的。**判据全绿不代表产出的文件能被解析**，
这两件事之间隔着"报告层有没有把括号配平"，当时没有任何一条判据在看那里。

**④ `/1`–`/2` 的证据没有 `av_drift.measurable` 这一格。**

判据 `drift.get("measurable", False)` 默认 False，于是
`p12c/superseded/p12c8-av-r1-sync.json`（180 个样本、带完整分布）
被判成"不可测量却没说理由"。这已经是既有缺陷 —— 用 `git show HEAD:` 的版本跑，同样红。

修法：缺格时按 `samples != 0` 推断，并把推断写进说明。
另外**全仓扫描明确跳过 `superseded/`**（那是已作废的证据；那一份顶层写
`clock_source=system` 而 180 帧走音频档，正是因为它自相矛盾才被归档的），
跳过份数在汇总行报出来。

---

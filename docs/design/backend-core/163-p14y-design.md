# 163 · P14-Y 设计：把"钟的不连续"从漂移里摘出来

- 日期：2026-10-31
- 前置：`162-p14x-status.md`（两次大修正都伴随钟事件；146.75 ms 里 113 ms 无证据）
- 目标：把钟的不连续**认出来、记下来、出版出来**，让那 113 ms 变成可复算的现场。
- 状态：**已收口（观测层）**，见 `164-p14y-status.md`。行为一层没改，理由见 §七.1 第三件。

---

## 一、要修的那句话

P14-X 证明的是**相关**：两次大修正（104.71 / 146.75 ms）的轮次都有
`extrapolated_frames = 1` + `monotonic_rejects = 1`，六轮正常轮全 0/0。

机制上的解释在这条式子里（`clock_position_tracker.cxx`）：

```cpp
raw_lead = position - (origin_position + wall_total);
```

`raw_lead` 是**派生量**，不是累加量。钟被拒一次（源拒了回退候选，位置冻结，墙照走），
这条式子立刻多出一段 `−wall`；钟恢复时再补回来 +113 ms，就变成一段 `+113`。
**两段都不是速率误差**，而旧报告里没有一格能区分"钟停了"与"钟快了"。

这一卡不改阈值，只把那两段记下来。

## 二、裁决表：什么算漂移，什么算不连续

`step = 位置实际推进 − 墙钟推进`。**一帧最多记一笔**，优先级从上到下。

| 事件（`read()` 里的分支） | 进 `offset`？ | 理由 |
| --- | --- | --- |
| `monotonic_rejected` | ✅ `step = −wall` | 源拒了回退候选，位置停住而墙走了 |
| `backward_clamped` | ✅ 同上 | 第二道守卫，钳掉的量同样是台阶 |
| `jump_rebased` | ❌ 恒 0 | 该分支已按墙推进并重锚，账上不该再动一次 |
| `extrapolated` | ❌ 恒 0 | 位置按墙推进，本身不产生台阶；它是"钟沉默过"的标记 |
| `position_step` | ✅ `step = raw − wall` | **正常分支里的台阶**：单帧偏离超过 30 ms |

第五类是设计时**漏掉、实现时才补上**的（见 §七.1）：钟停更之后补回来的那一帧
没有任何标志位，它落在正常分支里，只有比对增量才看得见 ——
P14-X 缺的 113 ms 正是这一帧。

## 三、文件家族树

全部落在既有目录，**不新建任何目录**。

```
core/pace/                                   （既有族：三之四）
├── pace_discontinuity.hpp              【新】词汇：五类事件 + 一笔现场 + 轨迹结构
├── pace_discontinuity_ledger.hpp       【新】账本接口：note / offset / trace
├── pace_discontinuity_ledger.cxx       【新】定长环 8 + 累计 offset + 分类计数
├── pace_rebase_trace.hpp               （P14-X，不动）
└── clock_position_tracker.hpp/.cxx     （改：一个成员 + 五处调用 + 一个分类函数）

backend/tests/video/pace/
└── p14y_discontinuity_tests/
    └── discontinuity_tests.cpp         【新】账本 + tracker 集成，9 个用例

docs/design/backend-core/
├── 163-p14y-design.md                  （本文）
└── 164-p14y-status.md                  （收口时写）
```

| 新增文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `pace_discontinuity.hpp` | 定义"有哪几种事件、一笔现场记什么、轨迹怎么摆"（纯数据，零依赖） | 不记账、不算 offset、不认识策略 |
| `pace_discontinuity_ledger.hpp` | 账本的契约 | 不做判定、不碰 `PacePolicy` |
| `pace_discontinuity_ledger.cxx` | 定长环 + 累计 + 升序展开 | 不改位置、不解释事件、不管出版格式 |

### 实际改动量（对预算，如实记）

| 文件 | 预算 | 实际 |
| --- | --- | --- |
| `clock_position_tracker.hpp` | +8 | +7 |
| `clock_position_tracker.cxx` | +14 | **+75 / −2** |
| `pace_stats.hpp` | +3 | +6 |
| `frame_pacer.cxx` | +2 | +3 |
| `av_sync_report.cxx` | +26 | +46 |
| `verify_p12c_sync.py` | +90 | +109 |
| `verify_p12_all.py` | — | +7（`/36` 进三处白名单） |
| `discontinuity_tests.cpp` | ~190 | 297 |
| 两份 CMake | +8 | +18 |

`tracker.cxx` 是唯一明显超的一处：**逻辑约 30 行，注释约 45 行** ——
分类函数要写清"为什么一帧只认一类"，`cum` 那行要写清"为什么没改行为"。
初稿那段注释是 11 行，后来压到 4 行、余下的推进 `163` §七.1。
这条记在这里而不是悄悄放过：预算是我自己定的，超了就得看得见。

## 四、出版（`sync/35` → `/36`）

```json
"drift_discontinuity": {
  "events": 2, "extrapolated": 1, "rejected": 0, "clamped": 0, "jumped": 0, "stepped": 1,
  "offset_ns": 113000000, "sampled_offset_ns": 113000000, "sampled": 2,
  "first_at_ns": 24400000000,
  "max": {"at_ns": 24430000000, "kind": "position_step",
          "wall_delta_ns": 33333333, "raw_delta_ns": 146333333, "step_ns": 113000000,
          "lead_before_ns": 33675000, "lead_after_ns": 146675000},
  "recent": [ ... 最多 8 笔，按时间升序 ... ]
}
```

- **两个原始量都出版**：`wall_delta_ns` 与 `raw_delta_ns`。只给 `step` 的话，
  "钟停了一下"（raw=0）与"钟补了一刀"（raw 很大）分不开。
- `sampled_offset_ns` 是环内 `Σstep`，与总账 `offset_ns` 分开：环只留 8 笔，
  两者只在 `events ≤ 8` 时相等。混成一个数会让"总账被环截断"这件事看不出来。
- **没有在 `clock_health` 里再印一份 `offset`**。设计稿原本想加两格，实现时发现
  那两格和 `drift_discontinuity` 出自同一个结构体，印两次只能证明"同一个变量被读了两次"。
  宁可少一格，也不要一格假的独立性。

## 五、判据（Y 组）

| 判据 | 内容 | 打红意味着 |
| --- | --- | --- |
| Y0 | 零事件 ⇒ `offset == 0`、`sampled == 0`、`recent` 空、五个分类全 0 | 幽灵账 |
| Y1 | `Σ recent.step == sampled_offset_ns`；且 `events ≤ 8 ⇒ sampled_offset == offset` | 环回绕时没扣掉被挤走那一笔 |
| Y2 | `events == 五个分类之和`；`sampled == min(events,8) == len(recent)`；`at_ns` 严格升序；`kind` 在白名单 | 一帧记了两笔，或漏写分类 |
| Y3 | 单笔修正 > 迟滞带 + 一帧 ⇒ 环里必须有一笔 `|step| ≥ 修正 − (带+帧)` | **那 113 ms 没有出处** |
| Y4 | `max` 必须是环里 `|step|` 最大的那一笔 | 最大值没跟着更新，读者会盯着一个小的 |

变异（`build/p14y-mutant-y3-sync.json` 已跑过一次）：把那一笔 `step` 抹成 0 而
保留 146.75 ms 的修正 ⇒ **Y3 当场红**，理由写明"仍有 108.42 ms 没有出处"。
正式卡要补 6–8 条变异；这里先立住最要紧的那一条。

## 六、预期（改过一次）

原设计写的是"门不再还台阶，146.75 ms 应消失"。**这条做不到**，因为那不归本卡管
（§七.1）。修正后的预期：

- 那笔 146.75 ms 的修正**仍会出现**（P13-S 的契约没动）；
- 但 `drift_discontinuity` 里必须有一笔 `|step| ≈ 113 ms` 与它同轮，
  于是"113 ms 从哪来"第一次有了现场可查；
- 正常轮：零事件、`offset = 0`，`drift_rebases` 与改动前逐位相同。

**证伪**：若真机上大修正那一轮 `offset` 只有几毫秒，说明台阶不在那一刻 ——
那么本卡的结论必须写成"不是不连续"，缺口留给下一卡。

## 七、不做的事

- **不动任何阈值**：`drift_limit_ns` / `drift_band_ns` / `jump_limit_ns` 一个不改。
- **不改门的输入**：`cum` 仍是 `raw_lead`。见 §七.1。
- 不做变长数组、不做跨会话累计、不新建目录。

### 七.1 实现时撞到的三件事（都要记住）

1. **第五类事件是补出来的**。原设计只有四类，全靠分支标志位。可"停更之后补回来"
   那一帧走的是正常分支，没有任何标志 —— 只认 flag 的话，113 ms 依然抓不到。
   于是加了 `position_step`，判定用文件里**早已存在却从没被用过**的 `kStepDriftNs`
   （P13-V 写下的 30 ms，当时只是意图）。
2. **Y1 必须拆成两条**。环只留 8 笔，`Σ recent.step` 天然不等于总账；
   原设计的 `Y1: Σrecent == offset` 在 `events > 8` 时永远红。
3. **"把台阶从 cum 里减掉"会推翻 P13-S/AB 的契约**。试了一版，`kivo_p12c5_pace_tests`
   立刻红 9 条 —— S5/S6/S7 三条**就是用"台阶必须被门还掉"写的**（S7 名字叫
   "时钟台阶注入且门只吃一次"）。那不是本卡能顺手改的：改它等于改产品行为，
   要有数据、要单独一张卡、要连 S5–S7 一起重写并留下理由。
   所以本卡退成**只观测**：账记全，门照旧还它。
   "该不该停止还台阶"留给下一张卡 —— 现在有账了，可以按数据判。

## 八、真机计划（已跑，见 164）

| 组 | 参数 | 为什么 |
| --- | --- | --- |
| r1–r4 | 4K30 60 s / 5000 ppm / 门开 | 与 P14-X 同参数，四轮逐格可直接比 |
| r5 | 同参数 / 门关 | 记账不该被门的状态污染（桌面已有 t07 守着） |

证据命名 `docs/evidence/p14y/p14y-4k30-r{1..5}[-sync].json`。
仍是一台设备（K40 `b6e950ee`），仍不拍 4K60 60 s。

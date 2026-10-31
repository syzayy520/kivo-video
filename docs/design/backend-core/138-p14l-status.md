# 138 · P14-L 验收：同一个 0，两个档里是两件不同的事

- 设计：`137-p14l-design.md`
- 证据：`docs/evidence/p14l/`（1080p30 / 60 s / 同一片源 / 只换排程档）
- 改动：`decode/17` → `/18` —— `drop_ratio` / `catch_up_*` / `over_budget_frames`
  各增 `_measured` + `_reason`
- 校验：`verify_p14i_gap` P1–P11 **4/4 份通过**；全量 `verify_p12_all` **640 份 / 四层全通过 / 20 个子校验器 exit 0**
- 门禁：`ctest -C Debug -R "kivo_p1[234]"` **25/25**

---

## 一、这一卡要证的一件事

> `over_budget_frames = 0` 在两个排程档里含义完全不同：一个是真读数，一个是空读数。

同台设备（K40 / `b6e950ee`）、同片源（`p13af_1080p30_60s_av.mp4`）、同参数，
只换 `kivo.present`：

| 项 | `cpu_paced` | `surface_at_time` |
|---|---|---|
| `frames_decided` / `presented` / `dropped` | 1800 / 1800 / 0 | 1800 / 1800 / 0 |
| `lateness_ns.samples` | **1800** | **0** |
| `lateness_ns.measured` | **true** | **false** |
| `over_budget_frames` | 0 | 0 |
| **`over_budget_frames_measured`** | **true** | **false** |
| `catch_up_measured` | true | true |
| `drop_ratio_measured` | true | true |

**同一个 0，左边是可读的「一帧都没超」，右边是空的「根本没量」。**

`over_budget_frames` 是 G6 的判据。改之前，右边那一列会让 G6 通过一次从未发生的检查 ——
而它看起来和左边一模一样。这就是 P14-K 状态卡 §三.2 留下的那一格。

---

## 二、三格的判断条件各不相同，但 `over_budget` 不独立判断

| 格 | 为 0 时的判别条件 | 判断位置 |
|---|---|---|
| `over_budget_frames` | `lateness.samples > 0` | **复制** `present_timestamp_measured` |
| `catch_up_events` / `catch_up_frames` | `frames_decided > 0` | `FramePacer::summary()` |
| `drop_ratio` | `presented + dropped > 0` | `PaceStats::summary()` |

第三列是本卡最容易做错的地方：`frames_decided` 由节拍器记账，
`PaceStats::summary()` 里那个字段恒为 0 —— 在那里判会**永远得到「没决策过」**。
所以 `catch_up` 放在 `FramePacer::summary()`，与 P13-M 的 `note_anchor_reset()` 同一个做法：
**问那个确实知道的地方。**

`over_budget` 则相反：它**不独立判断**，直接复制上游。
两条链各判一次，迟早会在某个边界上分家，而分家之后证据会同时说
「lateness 没量到」和「超预算帧量到了」—— 读者只会看见后者。

---

## 三、判据

| 编号 | 内容 | 结果 |
|---|---|---|
| P1–P6 | 接收圈结构（P14-I） | 4/4 通过 |
| P7 | `lateness.measured == (samples > 0)` | 4/4 通过 |
| P8 | `measured=false` 时 `schedule_lead.samples > 0` | 通过（1800 / 1800） |
| **P9** | `over_budget_frames_measured == lateness.measured`；`false` 时值必须为 0 | 通过 |
| **P10** | `catch_up_measured == (frames_decided > 0)`、`drop_ratio_measured == (presented+dropped > 0)` | 通过 |
| **P11** | `measured=true` 不许带理由；`false` 必须有理由且不能是默认证词 | 通过 |
| T1–T8 | 时钟侧（P14-G/J） | 4/4 通过 |

### 3.1 变异自测（正例绿 + 五个变异全红）

| 变异 | 期望 | 实际 |
|---|---|---|
| 正例（真机证据） | 绿 | `OK` |
| A 派生链分家（`over_budget` 自称量到、`lateness` 没量到） | 红 | **P9 打红** |
| B 没量到却报 `over_budget_frames = 7` | 红 | **P9 打红** |
| C `catch_up_measured` 与 `frames_decided` 不符 | 红 | **P10 打红** |
| D 理由写成那句默认证词 | 红 | **P11 打红** |
| E 量到了却仍带理由 | 红 | **P11 打红** |

D 与 E 第一版构造错了 —— 它们被更早的 P9/P10 拦下，没走到 P11。
把前提补齐（D 让分母真的为 0、E 让上游真的量到）之后才验到 P11 本身。
**"被别的判据拦下"不等于"这条判据有效"**，这一条要写在变异脚本的用法里。

---

## 四、诚实清单

1. **`sync` 报告（`kivo.p12c.sync/24`）的同一个缺陷没有修。**
   它的 `over_budget_frames` 与 `lateness_ns` 同样没有 `measured` ——
   `p14l-*-sync.json` 里 `over_budget_frames` 仍是裸 0。
   不修的理由是范围：一张卡动两份 schema 会让两侧判据的版本门槛同时抬升，回滚成本翻倍。
   **这是下一张卡的入口，不是"已经没问题了"。**
2. **G6 / G7 在默认排程档上仍然没有被验证。**
   这一卡让"没量到"变得显式，**没有让测量发生**。真要测到上屏时刻，
   得改呈现路径（合成器回调），那是独立的一项。
   本卡的产出只有一句：**"默认档的 G6 通过"这句话，从今以后不能再被引用。**
3. **`interval_ns` 没动。** 它在两档下都有样本，但 `surface_at_time` 下测的是
   **提交间隔**、`cpu_paced` 下测的是**上屏间隔** —— 同名字段跨档含义不同
   （P14-K §二 已指出 `pace_ns` 有同样的问题）。这是另一类缺陷，不在本卡范围。
4. **只有 K40 一台，每档只跑一轮。** 可测性翻转是结构性的（`note_presented` 有没有被调用），
   与设备无关；但"两档都 0 丢帧"这个数字只有一台机器的证据。
5. **`catch_up_measured` 在两档都是 true，而两档的 `catch_up_events` 都是 0。**
   这个 true 是对的（确实决策了 1800 帧），但 "0 次追赶" 在默认档里是**结构性必然**
   —— 提前 3 帧提交就永不迟到。这个 0 可读，但不证明"追赶机制没问题"。

---

## 五、结论

**三格沉默的零里，最要命的那一格（`over_budget_frames`）关掉了。**

改之前：一份 `surface_at_time` 证据里，`over_budget_frames: 0` 和
`over_budget_frames_measured: false` 长得一样好，而 G6 只读前者。
改之后：同一台设备、同一片源，`cpu_paced` 报 `true`、`surface_at_time` 报 `false` ——
**读者不需要读文档，两份证据并排放就看得见。**

这一格做完了，P14 系列收口。下一次碰这块，入口是 §四.1（`sync` 侧）与 §四.2（真正的上屏测量）。

---

## 六、下一张卡

**把 `MeasuredCounter` 接到 `sync` 报告（`kivo.p12c.sync/24` → `/25`）。**

理由：`av_sync_report.cxx` 里的 `over_budget_frames`（第 931 行）与 `lateness_ns`
（第 955 行）至今是裸数。同一台设备同一轮播放，两份报告会对同一个概念
说出两种可信度 —— 而读者没有理由怀疑其中任何一份。

`core::pace::MeasuredCounter` 已经在 `PaceSummary` 上，`av_sync_report` 拿到的正是
`PaceSummary`，接线成本比这一卡低。要新写的只有一组判据（S9–S11），
与 P9–P11 同形但不共用文件 —— 两份报告的证据门槛不同版本，判据不能混在一个校验器里。

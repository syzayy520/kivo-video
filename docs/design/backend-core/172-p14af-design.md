# 172 · P14-AF 设计：把"被丢掉的 1 s"搬到顶层（改口径）

- 状态：**已执行**，见 `173-p14af-status.md`
- 前置：`171-p14ae-status.md`（`lead = 0` 的两种来历）、`169-p14ad-status.md` §三

## 一、要改什么、为什么

P14-AE 量到：≥ 1 s 的跳变走 `jump_rebased` 分支，位置按墙推进、**台阶被丢掉**、`raw_lead` 归 0。
于是 `lead = 0` 有两种来历 —— "真对齐"或"被丢掉了" —— 而顶层只有 `jump_rebases` 一个计数，
被丢掉多少埋在 `recent[i]` 的 `raw_delta − wall_delta` 里。

两件事要补：

1. **出版 `drift_discontinuity.jump_max_ns`** —— 本次会话被丢掉的**最大那一笔**（带符号）。
2. **`jump_rebases > 0` 的轮次，`sync_verdict` 给 `unknown`** —— 那一轮的报告没有资格断言"同步"。

## 二、为什么 `unknown` 而不是 `clock_off`

`av_synced` 说"对上了"，`clock_off` 说"偏了"。跳变轮里**两个都不知道**：

- 若那 1.5 s 是 seek / 设备重建（合法不连续），视频跟着墙走是对的 ⇒ 实际同步；
- 若那 1.5 s 是钟错了，视频跟着墙走就偏了 1.5 s ⇒ 实际偏了。

同一份数据分不出这两种现场（`171` §五）。用 `clock_off` 等于替它选后者 —— 那也是一种编造。
`unknown` 是这一格里唯一诚实的取值，而 `sync_verdict_reason` 负责把那句话写出来。

## 三、改动清单与预算

| 文件 | 预算 | 改什么 |
|---|---|---|
| `pace_discontinuity.hpp` | +4 | `DiscontinuityTrace::jump_max_ns`（带符号） |
| `pace_discontinuity_ledger.hpp` | +4 | 一个成员 + 访问器注释 |
| `pace_discontinuity_ledger.cxx` | +8 | `note()` 里记"最大被丢弃量"；`trace()` 里填 |
| `av_sync_report.cxx` | +4 / +7 | 出版 `jump_max_ns`；`sync_verdict` 的跳变分支 |
| `verify_p12c_sync.py` | +26 | AF1 / AF2 |
| `p14y_discontinuity_tests` | +45 | 一个用例：跳变量记最大、带符号、reset 清零 |
| 三处 schema 白名单 | +3 | `sync/39` → `/40` |

**源码合计约 +23 行**（不含测试与判据）。全部落在既有节点里，**不新增文件、不新增目录**。

## 四、`jump_max_ns` 的语义（写死，免得下次又吵）

- **带符号**：`raw_delta − wall_delta` 的原值。正 = 钟往前跳（位置超前），负 = 往回跳。
  只给绝对值的话，读者分不出"钟快了"还是"钟慢了" —— 与 `clock_ahead` / `playback_rate`
  的符号约定一致：**正 = 快**。
- **取 `|·|` 最大的那一笔**，不是最后一笔，也不是累计。累计没有意义：跳变被丢弃后，
  下一笔又可能反向跳回来，加起来会互相抵消。
- **只有跳变进这一格**：`position_step` / `clamped` / `rejected` 那些走 `offset_ns`，
  两者不许混（`171` §五 的判据 AE2 守着这条）。

## 五、判据 AF1 / AF2

**AF1（顶层那一格必须能被账解释）**：`jump_rebases > 0` 且账上跳变笔数没有溢出环
（`jumped <= len(recent)`）⇒ `|jump_max_ns|` 必须等于环里 `|raw − wall|` 的最大值（±1 ms）。
溢出环时判不了，记 note —— 与 AD1 同一个处理方式。

**AF2（跳变轮不许说同步）**：`jump_rebases > 0` ⇒ `av_drift.sync_verdict` 必须是 `unknown`。
这是这一卡唯一改变语义的判据，也是最容易在未来被"顺手改回去"的一条。

变异（函数级，各一条）：跳到 1500 → `jump_max_ns` 写成 1200 必须红；
跳变轮的 verdict 写成 `av_synced` 必须红；无跳变的轮次两条都不许发声。

## 六、会变的东西与不会变的东西

| 项 | `/39` | `/40` |
|---|---|---|
| `drift_discontinuity` | 无 `jump_max_ns` | 多一格（带符号） |
| 跳变轮的 `sync_verdict` | `av_synced` | **`unknown`** |
| 非跳变轮的全部字段 | — | **逐位不变**（要有对照轮证明） |
| `lead_ns` / `clock_lead_ns` / `offset_ns` | — | **一字不动** |

**"非跳变轮逐位不变"是这一卡的验收条件之一**：改的是"多一句话"，不是"改一个数"。
做法：拿 P14-AD 的 14 份 `/39` 与**同参数重跑**的 `/40` 对照 —— 若同参数两版的
`clock_lead_ns` / `offset_ns` / `frames_dropped` 不完全相同，说明我碰到了不该碰的东西。

## 七、不做的事

1. **不改 `jump_limit_ns`**（悬崖仍在 1000 ms；要动它得单独一张卡 + 对照数据）
2. **不改 `lead` / `clock_lead` 的定义**（这一卡只加一格、改一格取值）
3. **不对 at_time 做特例**（`span_basis=scheduled` 的轮次同样适用）
4. **不追溯旧证据**：`/40` 之后才判；`/39` 及以前的 `av_synced` 仍是当时的真话

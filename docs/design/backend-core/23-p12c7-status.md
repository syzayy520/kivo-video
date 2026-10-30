# P12-C7 · 精确呈现时刻 · 验收与收口

- 版本：V1.0（结题）
- 日期：2026-10-29
- 上游：`22-p12c7-design.md`（设计与验收口径）、`21-p12c6b-present-backpressure.md` §六
- 下游：C8（A/V 合并取证）

---

## 一句话结论

**有效，已设为默认。**

把"CPU 睡到 `due_at` 再交"换成"不睡、把 `due_at` 交给合成器"之后，
**提交提前量在 12 次运行里 spread 全为 0**（p10 = p50 = p90 = −99.875 ms），
而旧结构在同样负载下交出错峰从 0.18 ms 涨到 **2.12 ms**。

但要说清楚它买到了什么：**买的是"不再受线程调度摆布"，不是"画面更流畅"。**
12 次运行全部 0 丢帧 —— 两种结构都没让任何一帧掉队。

---

## 一、判据（照设计稿，未放宽）

设计稿 §五 定了三选一，并规定主判据是**提交提前量的跨度**，不是均值、不是 fps。

| 结论 | 触发条件 | 本次 |
| --- | --- | --- |
| **有效** | 负载下跨度明显更稳、护栏全绿 | ✅ |
| 无差别 | 两模式指标在噪声内 | — |
| 更差 | 新增丢帧 / 提前量失控 | — |

护栏五项全部成立（12 次运行）：`frames_dropped_late = 0`、
`frames_released == frames_received`、`frames_rendered + 1 == frames_received`、
`over_budget_frames = 0`、`completed = true` 且 `error` 为空。

---

## 二、空载对照（3 轮 × 2 模式）

同 APK、同片源（`h264_1080p8.mp4`，30 fps）、不构造负载。

| 模式 | 轮 | `lateness` p99 | `lateness` max | 提交提前量 p50 | 跨度 | 丢帧 |
| --- | --- | --- | --- | --- | --- | --- |
| `cpu_paced` | 1 | 0.125 ms | 0.126 ms | —（无样本） | — | 0 |
| `cpu_paced` | 2 | 0.125 ms | 0.190 ms | — | — | 0 |
| `cpu_paced` | 3 | 0.125 ms | 0.299 ms | — | — | 0 |
| `surface_at_time` | 1 | 0（无样本） | 0 | **−99.875 ms** | **0** | 0 |
| `surface_at_time` | 2 | — | — | **−99.875 ms** | **0** | 0 |
| `surface_at_time` | 3 | — | — | **−99.875 ms** | **0** | 0 |

**−99.875 ms 正好是 3 个帧时长**（33.2917 ms × 3），即 `kMaxScheduleLeadFrames = 3` 的上限。
说明 cap 按设计生效：每帧都提前恰好 3 帧提交，不多不少。

> 两种模式的指标名不同（`lateness` vs `schedule_lead`）不是笔误：
> 前者是"睡到点之后迟了多少"，后者是"提前多久交出去"。**它们不能直接比大小**，
> 只能各自比自己的跨轮稳定性 —— 见 §四。

---

## 三、负载对照（3 轮 × 2 模式）

**负载**：5 个忙循环（`sh -c 'while true; do :; done'`），实测约 27% idle（正常 97%）。

| 模式 | 轮 | `lateness` p99 | `lateness` max | 提交提前量 p50 | 跨度 | 丢帧 |
| --- | --- | --- | --- | --- | --- | --- |
| `cpu_paced` | 1 | **0.875 ms** | **2.120 ms** | — | — | 0 |
| `cpu_paced` | 2 | 0.625 ms | 1.263 ms | — | — | 0 |
| `cpu_paced` | 3 | 0.375 ms | 1.342 ms | — | — | 0 |
| `surface_at_time` | 1 | 0（无样本） | 0 | **−99.875 ms** | **0** | 0 |
| `surface_at_time` | 2 | — | — | **−99.875 ms** | **0** | 0 |
| `surface_at_time` | 3 | — | — | **−99.875 ms** | **0** | 0 |

**旧结构的跨轮极差**：`lateness` p99 **0.500 ms**、max **0.857 ms**。
**新结构的跨轮极差**：提前量 p50 **0.0000 ms**、跨度 **恒为 0**。

负载把 `cpu_paced` 的交出错峰从 0.13–0.30 ms 抬到 1.26–2.12 ms（**7–16 倍**），
而 `surface_at_time` 一个纳秒都没动。

---

## 四、为什么「spread = 0」是强结论，不是巧合

因为它不是测出来的稳定，是**算出来的稳定**。

`kCpuPaced` 的提交时刻 = `sleep_until(due_at)` 醒来那一刻。醒来时刻由内核调度决定，
所以它天然带着调度器的方差 —— 空载 0.18 ms，加负载 2.12 ms，这是同一份代码的两种表现。

`kSurfaceAtTime` 的提交时刻 = `due_at − 3 × frame_duration`，**由算术决定**。
只要 `frame_duration` 不变（同一片源恒定），这个差值就一帧不差。
CPU 忙不忙、睡不睡得着，都不进入这个算式。

所以 spread = 0 是**结构性**的，不是这一轮运气好。三轮空载 + 三轮负载共六次全为 0，
两次不同负载强度下也全为 0，这与"算术恒等式"的预期一致。

**代价与边界**（设计稿 §4.3 已写，实测确认）：
提前量被 `kMaxScheduleLeadFrames = 3` 卡住，而 3 帧正好填满 Surface 的 BufferQueue。
实测 `release` 段最长一次阻塞 **12.03 ms**（负载第 2 轮），但因为提前了 3 帧（≈100 ms），
这一帧仍然在 `due_at` 之前交完，没有丢帧。**这就是提前提交买到的余量。**

---

## 五、默认值决策

**已把 `PipelineSetup::present_schedule` 的默认值改为 `kSurfaceAtTime`。**

改的不只一处，四处同时改（少改一处就会出现"文档说 A、APK 跑 B"）：

| 文件 | 改动 |
| --- | --- |
| `session/present_schedule.hpp` | 枚举注释改写为 C7 结论；`kSurfaceAtTime` 标为默认 |
| `session/decode_session.hpp` | `PipelineSetup::present_schedule` 与 `present_schedule_` 默认 `kSurfaceAtTime` |
| `jni/jni_session_handle.hpp` + `NativePlayer.kt` | `@param at_time` 语义与默认值说明同步 |
| `PlayerActivity.kt` | `atTimePresent` 默认 `true`；extra 改为 `--es kivo.present cpu_paced` 才回退 |

**为什么敢改默认**：

1. 12 次运行全部 0 丢帧，两种结构的正确性不变量一样绿；
2. 总时长没变长（`surface_at_time` 5.91–5.99 s，`cpu_paced` 6.02–6.07 s，前者略短）；
3. 显示时刻没有后移 —— 时间戳仍是 `due_at`，提前的只是"提交"这一步；
4. 回退成本为零：同一二进制传一个 extra 即可，不需要重编。

**风险与回退条件**：`releaseOutputBufferAtTime` 的排程语义依赖厂商实现，
本项目只在 K40 上验过。**换机后若出现帧数不闭合、`completed=false` 或画面停顿，
第一件事是把 `kivo.present` 设成 `cpu_paced` 复跑** —— 如果退回去就好了，那就是这条路径的问题。

---

## 六、这次没做到的（诚实清单）

1. **没测到"用户能感知的改善"。** 两种结构都 0 丢帧，所以差异只存在于
   "提交时刻的方差"这一层。说"C7 让画面更流畅"是**没有证据**的，本文不这么说。
> **2026-10-29 更正（P12-D5）**：本节第 6 条说过"上屏时刻在这个架构下拿不到"。
> **那是错的** —— `dumpsys SurfaceFlinger --latency` 要的是**带 `#id` 的完整层名**，
> 当年用的是去掉 `#id` 的名字，于是只返回刷新率一行，不报错、也不提示。
> D5 已用它测出：稳态 7 轮 **0 次错过 vsync**，Δ 上界恰好等于刷新周期。
> 详见 `34-p12d5-status.md` §一。本节其余条目不受影响。

2. **`schedule_lead` 是代理指标，不是上屏时刻。** C6B §二 已经证明在
   「纯 NDK + MediaCodec 直出 Surface」下拿不到 `presentedTimeNanos`
   （`dumpsys --latency` / `--frametimeline` 都不给数据，NDK 无对应 API）。
   本文能证明"提交更稳"，不能证明"上屏更稳"。后者要 present fence，留给 P12-D。
3. **负载是合成的**（忙循环），不等于真实场景（温控降频、GC、后台应用）。
   它只能证明方向，不能给出"在 XX 场景下会好多少"。
4. **`kMaxScheduleLeadFrames = 3` 是推出来的**（BufferQueue 通常 3–4 槽），
   没有做 1/2/4 的对照。实测 3 帧下 12 次运行 0 丢帧、`release` 最长阻塞 12 ms 仍能吸收，
   但没有证据说 2 就不行、4 会更好。
5. **只测 1080p。** 4K 的瓶颈在解码器（7.6 ms/帧，见 `20-p12c6-status.md`），
   排程方式改不动它，跑 4K 只会得到"无差别"。
6. **`at_time` 路径下 `lateness` 恒为 0。** 这不是"零迟到"，是"这条路径没有迟到样本"
   —— `arrive()` 不睡，`note_presented()` 不被调用。读证据时看 `schedule_lead_ns`，
   不要看 `lateness_ns`。这一点已在 schema `/4` 的字段注释里写明。

---

## 七、证据清单

`docs/evidence/p12c/`（K40 `b6e950ee` 实测，schema `kivo.p12c.decode/4`）：

- 空载 `cpu_paced`：`p12c7-idle-1080p30-cpu_paced-r{1..3}.json`
- 空载 `surface_at_time`：`p12c7-idle-1080p30-at_time-r{1..3}.json`
- 负载 `cpu_paced`：`p12c7-load-1080p30-cpu_paced-r{1..3}.json`
- 负载 `surface_at_time`：`p12c7-load-1080p30-at_time-r{1..3}.json`
- 开关首验：`p12c7-probe-at.json`

**取证方式**（默认已是 `surface_at_time`，所以复跑旧结构要显式指定）：

```bash
# 默认（surface_at_time）
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity \
  --es kivo.media $MEDIA/h264_1080p8.mp4 \
  --ei kivo.pace 1 --ei kivo.drop 1 --ei kivo.fps 30 \
  --es kivo.evidence p12c7.json"

# 回退到 C5 的睡眠结构
... --es kivo.present cpu_paced --es kivo.evidence p12c7-cpu.json
```

单测：`kivo_p12c5_pace_tests` 21 例 / 86 断言，含 4 条 C7 用例
（不睡但到期时刻不变 / 提前量不污染 lateness / 默认仍会睡 / 跨度排除启动瞬态）；
`ctest -R kivo_p12c` **5/5**。

---

## 八、给 C8 的接口

C8 的证据 JSON 需要带上：

- `pipeline.present_schedule`（已有，`"cpu_paced"` | `"surface_at_time"`）
- `pace.schedule_lead_ns`（已有，含 `samples/p01/p10/p50/p90/p99/min/max/spread`）

**不要**把 `lateness` 与 `schedule_lead` 合成一个"抖动"字段。
前者在 `surface_at_time` 下恒为 0（无样本），合并会让 C8 的 A/V 偏差统计
凭空多出一个"零抖动"的假结论。

# P13-drift · `surface_at_time` 模式下的漂移口径（提案）

日期：2026-11-09
来源：[P240-B 播放卡证据](../../evidence/p240/network-native-playback/mkv-ffmpeg-demux-r4-success.md)
状态：**提案，未实现**。归属 P13 drift 体系，不在播放卡内顺手改。

## 一、问题

`surface_at_time` 呈现模式下，`drift` 的采样与纠偏**完全没有运行**：

```json
"drift_rebase_trace": {"rebases": 0, "sampled": 0, "max": {"amount_ns": 0}},
"drift_gate": {"enabled": true, "limit_ns": 45000000, "band_ns": 5000000},
"av_drift": {"lead_ns": -645957004, "sync_verdict": "unknown"}
```

四份独立会话（446 s / 480 s ×3）里 `sampled` 与 `rebases` **始终为 0**，
而 `clock_lead` 从 0 线性累积到 −646 ~ −827 ms。

同一份报告里已有解释：

> "no present-timestamp samples: in surface_at_time the compositor decides when a
> frame is shown, so only the submit lead (schedule_lead_ns) is observable"

即：**帧何时真正上屏由合成器决定，我们拿不到 present 时间戳**；而 drift 的样本
来源正是 present。样本为零 → 纠偏为零 → lead 无界累积。

## 二、这不是"读时钟读错了"

- `clock_source = audio_timestamp`，视频跟音频，**音画同步正确**；
- 累积的是"媒体时间与墙钟的差"，不是"音画之间的差"；
- 20 s 窗口累积 ~40 ms 看不出来，480 s 累积 600–800 ms 才撞上断言。

所以它影响的不只是测试：**任何按墙钟做的调度（定时器、外部同步、进度显示校准）
都会带着这个偏移工作**。

## 三、现有机制的边界

`drift_gate` 是开着的（`limit_ns = 45 ms`、`band_ns = 5 ms`），但它的输入是 present 样本。
在 `surface_at_time` 下这个输入根本不存在，于是"门开着、没人喂数据"。

**结论：不是门限设错，是这一模式缺一条可观测的 lead 来源。**

## 四、两个候选方案

### 方案 A：用 `schedule_lead_ns` 驱动（推荐）

`schedule_lead` 是"提交时刻与应呈现时刻之差"，本模式**有样本**
（同一份报告：`"schedule_lead_ns": {"samples": 2, "p50": -119875000}`）。

- 改动点：drift 采样源从 present 改为 submit；判定式与 rebase 动作不变；
- 代价：submit lead 含排队与合成延迟，绝对值会比 present lead 大且更抖，
  `band_ns` 需要重新标定（不能沿用 5 ms）；
- 风险：若把合成器排队时间当成时钟误差去 rebase，会**反过来扰乱正确的时钟**——
  这是这个方案最需要证据的地方。

### 方案 B：直接用 `audio_position − wall_clock` 算

不依赖任何帧时间戳，纯用音频位置与单调墙钟的差。

- 优点：与呈现模式完全解耦，`surface_at_time` / `surface_early` 都能用；
- 代价：这条差里混入了硬件时钟的固有偏差（0–0.2%）与真实卡顿，
  两者必须能分开，否则 rebase 会把"硬件慢"误判成"我们漂了"；
- 前置：需要先有"本机音频时钟相对墙钟的稳定偏差"基线，否则无法设门限。

## 五、建议

先做**方案 B 的测量部分**（只观测、不 rebase）：连续记录 `audio_position − wall_clock`
的曲线，确认它是否稳定线性。
- 若稳定线性 → 那是硬件偏差，应当**用速率补偿**（ppm）而不是 rebase 跳变；
- 若出现台阶 → 那才是需要 rebase 的真漂移，方案 A 才有意义。

**在拿到这条曲线之前，两种方案都无法判定谁对。** 这也解释了为什么本卡不直接改：
现在改任何一边都是猜。

## 六、验收判据（提案）

| 判据 | 目标 |
| --- | --- |
| `sampled > 0` | `surface_at_time` 下必须产生样本 |
| `sync_verdict` | 不再是 `unknown`（当前恒为 unknown，因为没样本） |
| 长会话 `clock_lead` | 480 s 会话内保持在 `limit_ns` 内（不靠放宽界） |
| 音画同步 | 不得因 rebase 引入可见偏移（需要独立判据，不能只看数字） |
| 证据 | `drift_rebase_trace` 在 `surface_at_time` 下有非零 `sampled` |

## 七、不许做的

- 放宽 `limit_ns` 或测试的 `bound` 让它通过；
- 在播放卡里顺手改 drift（两套口径会打架）；
- 把 `sync_verdict=unknown` 记成"同步正常"——它是"测不出来"，不是"没问题"。

## 八、测量结果（2026-11-09 22:05，已有数据回算）

提案 §五 要求"先只测不纠"。这一步**不需要新代码**：`NetworkPlaybackSession.paceCurve()`
每秒记一次 `(墙钟ms, 媒体s)`，8 分钟会话的 `media_curve` 就是那条曲线。回算结果：

| 墙钟 s | 媒体 s | drift ms |
| --- | --- | --- |
| 0.0 | 0.00 | 0.0 |
| 60.9 | 60.86 | −67.1 |
| 121.8 | 121.58 | −204.7 |
| 243.4 | 243.03 | −358.7 |
| 364.9 | 364.41 | −540.3 |
| 480.4 | 479.74 | −648.8 |

分段漂移率：`0→122 s = −1681 ppm`，`122→304 s = −1356 ppm`。

### 结论：稳定线性，不是台阶

- 漂移**持续为负、近似恒定速率**（−1300 ~ −1700 ppm ≈ 0.13–0.17%）；
- 没有台阶、没有突变、没有"某一段突然掉"。

按 §五 的判据：**稳定线性 → 这是硬件时钟的固有偏差，应当用速率补偿（ppm）处理，
不是 rebase 跳变**。方案 A（拿 submit lead 去 rebase）因此**不成立**——
它会把这个物理偏差当成"我们漂了"去纠正，反而破坏正确的时钟。

### 真正的问题在判据，不在时钟

测试的断言是**绝对界**：

```kotlin
val bound = drift_gate.limit_ns + pace.frame_duration_ns   // 45ms + 41.7ms ≈ 86.7ms
check(abs(clock_lead_ns) <= bound) { "clock lead exceeds bound" }
```

而 `clock_lead` 是**累积量**：0.15% × 480 s ≈ 720 ms，**必然**越过 86.7 ms 这个固定值。
窗口越长越必挂，与实现质量无关。

项目其实已经有 ppm 口径——`drift_gate` 里就带着 `limit_ppm_equivalent`
（其注释写的是 "limit_ns / span"）。这一轮该做的是让**长窗口判据走 ppm**，
而不是继续用绝对界去卡一个物理上必然累积的量。

### 建议（替代 §四 的两个方案）

1. **判据**：长窗口用 `|clock_lead| / span` 与 ppm 界比较（与 `limit_ppm_equivalent` 同口径），
   绝对界只用于短窗口（≤ 60 s）。
2. **不 rebase**：本机音频时钟稳定偏慢，rebase 只会制造跳变。
3. **若要补偿**：只能通过 ppm 速率修正（`clock_injection.rate_ppm` 已有通道），
   且必须先用多轮会话证明该 ppm 在设备间稳定——单机单次不足以定标。

### 边界

- 本结论基于**单机（K40）四次会话**，其中三次 `rate≈0.998`、一次 ≈0.999。
- 平板上未做同口径回算，不能假定同一 ppm。
- `limit_ppm_equivalent` 的具体值与可用范围需要按窗口重算，本文未定标。

# 174 · P14-AG 设计：让"最差的那一下"说出自己在第几秒

- 状态：**已执行**，见 `175-p14ag-status.md`
- 前置：`169-p14ad-status.md` §九（"`present_jitter` 只有分位、问不了那 24 帧丢在哪几秒"）

## 一、缺口

`present_jitter` 与 `dropped` 都只有直方图（p50/p99/max + 计数）。直方图回答"多少、多严重"，
**不回答"在哪儿"**。于是 P14-AD 的 800 ms 那轮丢的 24 帧，只能知道"有 24 帧"，不知道
它们是在注入后 100 ms 内一次性丢的，还是分散在整段里 —— 这两种形状对判读完全不同：

- 一次性丢 ⇒ 台阶被"跳掉"，之后恢复正常；
- 分散丢 ⇒ 有持续的追赶，说明基准一直在飘。

`lateness` 侧早有 `WorstEpisode`（P13-P，最差帧 + 前后邻帧）；`present_jitter` 与 `dropped`
这两条一直没有对应的东西。**这一卡只补这两条**，不动 `lateness` 那一套。

## 二、改什么

| 位置 | 改动 |
|---|---|
| 新节点 `core/pace/pace_drop_trace.hpp` | `DropSample{media_ns, lateness_ns}` + 定长环 16 + `total/sampled/truncated` |
| `PaceStats` | `note_dropped(lateness, media_ns)`；`note_interval(deviation, media_ns)`；各留一个"最差样本" |
| `FramePacer` | 两个调用点各多传一个 `media_ns`（已有，不新增读取） |
| 报告 | `pace.dropped_events`（数组 + total/sampled/truncated） |
| 判据 | **AG1**（环与计数自洽）/ **AG2**（`dropped>0 ⇒ dropped_events 非空`） |
| 单测 | `p14y` 里加 t11：环只留 16、`truncated` 在 17 笔时置位、`reset` 清零 |

**为什么只有 16 格**：与 `RebaseTrace`(8) / `DiscontinuityTrace`(8) 同一个取舍 ——
"形状"只需要最近若干笔；`total` 与 `truncated` 负责说明"还有多少没留下"。
一次丢 24 帧（AD 的 800 ms 那轮）会留下最后 16 笔 + `truncated = true`，形状仍然看得出来。

**为什么用 `media_ns` 而不是单调钟**：问的是"第几秒"，媒体时间才是那个坐标系；
单调钟在不同会话里差着开机时长，列出来没法读（与 `DriftExtreme` 的选择一致）。

## 三、判据

**AG1**：`dropped_events.sampled == min(total, 16)`；`truncated == (total > 16)`；
数组按时间升序；每条 `|lateness| > late_drop_ns`（丢的定义就是这样）。

**AG2**：`frames_dropped > 0` ⇒ `dropped_events.total > 0`。**这条是本卡的意义**：
上一版里 `dropped` 只有计数，一条证据说"丢了 24 帧"而没有任何现场 —— 那就是"沉默的计数"。

变异：把 total 写成 0（应红）、把 sampled 写成 17（应红）、把一条 lateness 写成 10 ms（应红）。

## 四、会变与不会变

| 项 | `/40` | `/41` |
|---|---|---|
| `pace.dropped`（直方图） | 有 | **一字不动** |
| `pace.dropped_events` | 无 | 新增 |
| `present_jitter` 分位 | 有 | **一字不动**，另加 `worst` 一格 |
| 其余字段 | — | 不变 |

### 四之二、`present_jitter.worst` 为什么不在这一卡里

写设计时我打算一起做，读代码之后撤了：`render_quantiles` 的既有约定是
**`worst.lateness_ns` 必须与同口径直方图的 `max_ns()` 逐位相等**（判据 F1 在守它），
而 `present_jitter` 的样本是**有符号**的（间隔 − 期望），"最差"应当是 **|·| 最大**。
两者在负向样本上会分叉：一份 `min = −80 ms, max = +5 ms` 的证据，"最差"是 −80，
但 `max_ns()` 是 +5 —— 同一格里两个数打架。

这需要单独决定："这一格的 worst 是 max 还是 max-abs"，以及判据怎么写。
**不在这一卡顺手改**，因为它会影响 `av_drift` 那三个块的读法。

## 五、不做的事

1. **不做逐帧全量时间轴**：那是另一个量级的内存与出版成本，且"最近 16 笔 + 计数"已经能回答
   §一 的那个问题。要全量，等有具体问题再说。
2. **不动 `lateness` 的 `WorstEpisode`**（P13-P 的四个字段被 13 份老证据引用，判据 F1 在守）
2.1 **不做 `present_jitter.worst`**（理由见 §四之二）
3. **不改丢帧判据**（`late_drop_ns` 仍然 100 ms）

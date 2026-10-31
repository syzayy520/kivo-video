# P14-K 状态卡：默认排程档第一次被测 —— 也第一次露出两个"沉默的零"

- 设计：`135-p14k-design.md`
- 证据：`docs/evidence/p14k/`（1080p30 60 s / 4K60 8 s / 720p30 1800 s，全 `surface_at_time`）
- 改动：`decode/17` —— `pace.lateness_ns` 增 `samples` / `measured` / `reason`
- 校验：`verify_p14i_gap`（P1–P8）**3/3**、`verify_p14g_tail`（T1–T8）**3/3**
- 门禁：`ctest -R "kivo_p1[234]"` **25/25**

## 一、A/B：同片源、同参数，只换排程档

| 项 | `cpu_paced` 60 s | `surface_at_time` 60 s | `cpu_paced` 30 min | `surface_at_time` 30 min |
| --- | --- | --- | --- | --- |
| 帧数 | 1800 | 1800 | 54000 | 54000 |
| 丢帧 | 0 | **0** | 0 | **0** |
| 最长一圈 | 34.9 ms | 36.5 ms | 41.9 ms | **51.1 ms** |
| 该圈 pace | 30.9 | 32.9 | 31.4 | 33.1 |
| 该圈 release | 1.4 | 3.2 | 8.7 | **17.8** |
| 进程 CPU | 14.7% | 14.1% | 14.6% | 14.1% |
| 提交提前量 p50 | — | **−99.9 ms**（spread 0） | — | **−99.9 ms**（spread 0） |
| 时钟：钳制 / 长间隔 | 0 / 1（收尾） | **0 / 1（收尾）** | 0 / 1（收尾） | **0 / 1（收尾）** |

`−99.9 ms` 是 30 fps 下**正好 3 帧**（`kMaxScheduleLeadFrames = 3`），`spread = 0`
说明每一帧提前得一样多 —— 这正是这一档的设计目标。

4K60（480 帧，8 s）：**0 丢帧**、最长一圈 19.8 ms、release 峰值 3.4 ms、进程 CPU 20.4%。
这是"合成器定时在 4K60 下还成立"的第一份证据。

## 二、设计卡的第一条期望错了

设计卡 §一 写的是"`present_gap.worst_pace_ns` 在 `surface_at_time` 下**本该接近 0**"。
实测是 **32.9 ms**（`cpu_paced` 30.9）—— 几乎没变。

原因在代码里：`surface_at_time` 并不是"完全不睡"，它在 `handle_frame()` 里
`sleep` 到 **`due_at − cap`**（cap = 3 帧），好让提交提前量恒定（`worker.cxx:120`）。
所以 `pace` 里仍然有约一帧的睡眠，只是睡的目的从"等到该显示"变成"等到最早可提交"。

**这不是缺陷，是我把设计意图记错了。** 留着这段是因为它说明：
`pace_ns` 这个字段在两档里的**含义不同**（一档是"等显示"，一档是"等可提交"），
拿它跨档比较大小是没有意义的。

## 三、两个"沉默的零"（本卡真正的收获）

### 1. `lateness` 在默认档零样本，却印成 0

| 项 | `cpu_paced` | `surface_at_time` |
| --- | --- | --- |
| `note_presented()` 被调用 | 是 | **否**（只调 `note_scheduled()`） |
| `lateness_ns.p50/p99/min/max` | 有值 | **全 0** |
| `sampled_at_frames` | 1800 | **1800** |

读者看到的是"lateness 0、1800 帧采样" —— 结论会是"完美同步"。实际**一个样本都没有**。

这意味着：**G7（`p99 < 40 ms`）从来没在默认档上验证过。** 之前所有
"G7 ✅ 0.125 ms"都出自 `cpu_paced`，而默认档恰恰不是它。

已修（`decode/17`）：出版 `samples` / `measured` / `reason`。
新判据 **P7** 抓"零样本却自称测到"，**P8** 要求此时至少有 `schedule_lead.samples > 0`
（否则这一档的时点证据完全空白）。

### 2. `over_budget_frames` 也是 0，而且同样是派生的

`pace_stats.cxx:235`：`over_budget_frames = lateness_.count_abs_beyond(frame_duration_ns)` ——
**从 lateness 直方图派生**。lateness 没样本，它必然为 0。

这一格**本卡没有修**（只加了 `lateness.measured`）。理由是同一类修改应该一次做完：
`over_budget` / `catch_up` / `drop_ratio` 都可能在某档下退化成 0，
要加就一起加 `measured` 标志，而不是打一个补丁留三个。已写进下一张卡。

## 四、判据与门禁

| 编号 | 内容 | 结果 |
| --- | --- | --- |
| P1–P6 | 分档/计数/残差/极值/嵌套段（P14-I） | 3/3 通过 |
| **P7** | `measured == (samples > 0)`，`false` 时必须有 `reason` | 2/2（`/17` 的 2 份）通过 |
| **P8** | `measured == false` 时 `schedule_lead.samples > 0` | 通过（1800 / 500） |
| T1–T8 | 时钟侧（P14-G/J） | 3/3 通过 |

`p14k-long1800` 产生于 `decode/16`（改动前的二进制），所以 P7/P8 对它显式跳过 ——
**不重写历史证据**，状态卡里注明它的数值与 `/17` 等价。

## 五、诚实清单

1. **4K60 只有 8 秒**（480 帧）。够证明"不崩、不丢帧"，不够证明"能撑两小时"。
2. **`over_budget_frames` 在默认档仍是沉默的零**（见 §三.2）。
3. 只有 K40 一台。`surface_at_time` 的行为高度依赖合成器与 GPU 驱动，**跨机型最可能变**。
4. 这一轮没有测 `kivo.render 0`（纯解码）与 `kivo.threads 1`（分离管线）。

## 六、结论

**默认排程档是好的**：三份证据、56288 个时钟样本、0 丢帧、0 钳制、提前量 spread 0，
4K60 也不丢帧。它比 `cpu_paced` 略差的两格（最长一圈 51.1 vs 41.9、release 峰值 17.8 vs 8.7）
是"提前 3 帧提交"的必然代价，换来的是"上屏时刻由合成器保证"。

**但它的监控是残缺的**：G7 与 G6 的两个关键计数在这一档里都是 0，
而 0 看起来永远是好消息。修完 `measured` 标志之前，
"默认档达标"这句话只能说一半。

## 七、下一张卡

**给所有派生计数加 `measured` / `reason`**：`over_budget_frames`、`catch_up_events`、
`drop_ratio`、`lateness_*` 一起，用同一套写法（`measured` + 一句话理由），
并让 `verify_p14i_gap` 的 P7/P8 覆盖它们。理由是 §三.2 —— 同一个病要么一次治完，
要么会被下一张卡再犯一次。

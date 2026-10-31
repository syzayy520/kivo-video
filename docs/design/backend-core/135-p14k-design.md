# P14-K 设计卡：`surface_at_time` 排程档 —— 换一档，仪器还成立吗

- 上游：`133-p14i-status.md` §六.3、`134-p14j-status.md` §六.1
- 证据：`docs/evidence/p14k/`（1080p30 60 s / 4K60 8 s / 720p30 1800 s，全 `surface_at_time`）
- 判据：复用 `verify_p14g_tail`（T1–T8）与 `verify_p14i_gap`（P1–P6）

## 一、为什么这一卡不能省

P14-A…J 的**每一个结论**都只在 `cpu_paced` 一档上：

| 结论 | 出自 | 只在 cpu_paced 上成立吗 |
| --- | --- | --- |
| 设备 2 小时零跳变 | P14-G | 时钟侧，应该无关 |
| 钳制归零 | P14-H | 时钟侧，应该无关 |
| 循环最长 41.9 ms | P14-I | **强相关**：`cpu_paced` 里 `arrive()` 会睡到 `due_at` |
| 长间隔在收尾 | P14-J | 与排程无关（收尾路径） |

而 `surface_at_time` 是**默认档**（`atTimePresent = extra != "cpu_paced"`，
`PlayerActivity.kt:484`）。也就是说：**我一直在测的其实是非默认的那一档。**

两档的机制完全不同：

| | `cpu_paced` | `surface_at_time` |
| --- | --- | --- |
| 谁负责按时 | 解码线程 `arrive()` 里睡 | 合成器（`releaseOutputBufferAtTime`） |
| 提交提前量 | 0（醒了立刻交） | 最多 3 帧（`kMaxScheduleLeadFrames`） |
| `present_gap` 的构成 | 送包 + **睡眠** + 交帧 | 送包 + 交帧（几乎不睡） |
| `pace_ns` 的含义 | 节拍等待（有意的） | 只剩时钟查询 + 判定 |

所以 `present_gap` 里的 `worst_pace_ns` 在 `surface_at_time` 下**本该接近 0**。
它若不为 0，说明 `arrive()` 在这档里也在睡 —— 那是实现与设计不一致。

## 二、三个用例各回答一个问题

| 用例 | 片源 | 回答 |
| --- | --- | --- |
| `p14k-smoke-1080p30` | 1080p30 / 60 s | 与 `p14i-smoke`（同片源、`cpu_paced`）**逐项 A/B** |
| `p14k-4k60` | 4K60 / 8 s（480 帧） | 最吃紧的一档：`releaseOutputBufferAtTime` 在 4K60 下还成不成立 |
| `p14k-long1800` | 720p30 / 1800 s | 与 `p14i-long1800` A/B：长跑稳态 |

## 三、要看哪几格

| 格 | 期望（`surface_at_time`） | 不满足时说明什么 |
| --- | --- | --- |
| `pipeline.present_schedule` | `surface_at_time` | 参数没生效，整轮作废 |
| `present_gap.worst_pace_ns` | **远小于** `cpu_paced` 的 31 ms | `arrive()` 在这档仍在睡 |
| `present_gap.max_gap_ns` | 不比 `cpu_paced` 差（41.9 ms） | 提交链变慢 |
| `pace.schedule_lead_ns` | 非 0（有提前量） | 排程没生效 |
| `pace.dropped` / `late` | 0 | 合成器没按时取 |
| `tail.long_gaps` / `clamps` | 与 `cpu_paced` 同量级（收尾 1 次 / 钳制 0） | 时钟侧被排程影响（不该） |

**不预设结论**：这一档可能更好（不睡 → 循环更快），也可能更差
（提前 3 帧提交 → BufferQueue 更满 → `release` 阻塞）。两种都要如实写。

## 四、判据 = 复用，不新写

这一卡**不加新判据、不改代码**（除非发现实现与设计不一致）。原因：
- 结构判据（分档、计数、残差非负）与排程档无关，`verify_p14g_tail` / `verify_p14i_gap` 直接适用；
- 一旦为这一档新写判据，就会出现"两套判据各说各话"，那正是 P13-AE 修过的病。

若某个判据在 `surface_at_time` 下**恒为假**（例如 `pace` 恒为 0 导致某条比例判据除零），
那要改的是判据的适用条件，不是放宽阈值 —— 并且要在状态卡里写清改了哪一条、为什么。

## 五、诚实边界

1. 4K60 片源只有 8 秒（480 帧）。够看"会不会崩"，不够看"能撑多久"。
2. 只有 K40 一台。`surface_at_time` 的行为高度依赖合成器与 GPU 驱动，**跨机型最可能变**。
3. 这一档不测 `kivo.render 0`（纯解码）、不测 `kivo.threads`（分离管线）。

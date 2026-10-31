# P14-I 设计卡：一次停顿由谁构成

- 上游：`129-p14g-status.md` §六.4、`131-p14h-status.md` §七（停顿 200–380 ms，位置不明）
- 落位：`core/pace/present_gap.{hpp,cxx}`；接线 `decode_receive_worker.cxx`；证据 `kivo.p12c.decode/16` 的 `metrics.present_gap`

## 一、先用已有数据算一遍，算出"算不出来"

四轮 30 分钟（P14-G）与 P14-H 那一轮，`pipeline` 里已有三段耗时峰值：

| 轮 | 停顿（间隔） | receive 峰值 | release 峰值 | submit 峰值 | 三段之和 | 能解释吗 |
| --- | --- | --- | --- | --- | --- | --- |
| r1 | 276 ms | 64.4 ms | 74.8 ms | 116.7 ms | 256 ms | 勉强 |
| r2 | 383 ms | 16.7 ms | 15.9 ms | 83.5 ms | 116 ms | **不能** |
| r3 | 253 ms | 8.2 ms | 12.6 ms | 21.1 ms | 42 ms | **不能** |
| r4 | 191 ms | 5.2 ms | 8.5 ms | 11.4 ms | 25 ms | **不能** |
| P14-H | 234 ms | 20.9 ms | 29.3 ms | 13.5 ms | 64 ms | **不能** |

而且这个加法**本身不成立**：三段的峰值可能发生在三个不同时刻，相加只是一个上界估计。
`pace.wait` 只有 `count` 与 `total_ns`（r3 平均 32 ms/帧，正是正常节拍等待），
**没有 max**，所以"是不是节拍器自己睡过去了"也答不了。

结论：要回答"停顿由谁构成"，必须**逐次**记账，而不是事后拿几个峰值拼。

## 二、要记的东西：一次迭代的四个数

接收线程的一圈是：

```
receive(阻塞等帧) → arrive()（内含时钟查询 + 按节拍睡） → release(交帧)
```

相邻两圈的起点之差就是"间隔"，它必然由这三段 + 一段"说不清的"组成：

```
gap(n) = receive(n) + pace(n) + release(n) + unaccounted(n)
```

`unaccounted` 不是"误差"，它是**我们要找的那个量**：如果它接近 gap，
就说明时间花在我们完全看不见的地方（调度被抢占、被别的线程/进程挤掉）。

## 三、只记最差的一次，不记平均

平均值会把 234 ms 抹成 32 ms —— 而 32 ms 恰恰是正常节拍等待，看起来"完全健康"。
所以 `PresentGapFacts` 只留：

| 字段 | 含义 |
| --- | --- |
| `iterations` | 圈数 |
| `long_gaps` | gap ≥ 100 ms 的圈数 |
| `max_gap_ns` | 最长的一圈 |
| `worst_receive_ns` / `worst_pace_ns` / `worst_release_ns` | **那一圈**的三段耗时 |
| `worst_unaccounted_ns` | 那一圈的残差（≥0） |
| `buckets[]` | gap 的分档分布（8/16/33/66/133/266/532/1000 ms 起） |

**为什么"那一圈"很重要**：三个峰值分开看没有意义（第一节的加法就错在这），
只有同一次迭代里的三个数放在一起，才叫"构成"。

## 四、判据（`tools/verify_p14i_gap.py`）

| 编号 | 判据 | 抓什么错 |
| --- | --- | --- |
| P1 | `iterations > 0` | 没测到 ≠ 没停顿 |
| P2 | `long_gaps <= iterations` | 分档/计数漏账 |
| P3 | `Σ buckets == iterations` | 加档忘扩数组 |
| P4 | `worst_unaccounted_ns <= max_gap_ns` | 减法写反（负残差被当 0 吞掉是最隐蔽的一种） |
| P5 | `max_gap_ns` 所在的档非空 | 极值与分布必须同一批样本 |

## 五、桌面单测（`kivo_p12c5_pace_tests`）

| 例 | 断言 |
| --- | --- |
| G1 | 三段占满整圈 → `unaccounted = 0` |
| G2 | 只有 1/4 被三段解释 → `unaccounted` 吃掉 3/4 |
| G3 | 分段耗时之和 > gap（时钟读得比实际粗）→ `unaccounted` 钳到 0，不出现负数 |
| G4 | 最差的一圈是**四段同时**记录的，不是各段各自的最大值 |
| G5 | 分档边界左闭右开，且 `Σ buckets == iterations` |

## 六、诚实边界

1. `pace(n)` 包含"时钟查询 + 决定 + 睡眠"，睡眠是**有意的**。所以
   `worst_pace_ns` 大不代表有问题 —— 它代表"这一圈本来就在等节拍"。
   判读时必须先看它有没有吃掉整圈。
2. `unaccounted` 大只能说明"不在我们调用的三段里"，**不能直接归因给调度** ——
   也可能是我们漏了一段没测（比如 `signal_input_room`、日志、JNI 回调）。
3. 仍然只有 K40 一台、720p30 一条片源。

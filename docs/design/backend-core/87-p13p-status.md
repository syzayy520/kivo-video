# 87 · P13-P 验收：把迟到拆成「手慢了」和「表快了」

- 版本：V1.0
- 日期：2026-10-30
- 设计：`86-p13p-design.md`
- 判据：P1–P6（见设计 §六）
- 结论：**工具成立，假设被推翻**。恒等式在 13/13 份真机证据上复算通过，
  正向对照判得对；但"88 ms 来自时钟侧"这条推断在真机上**没有发生**，
  12 轮 4K60 里 `|due_step| ≤ 36 µs`，唯一一次 45.636 ms 干净地落在 `present_side`。
  P13-F 的「片中抢占」措辞撤回，改为「首帧排空（未证实）」。

---

## 一、改了什么（一句话版）

以前 `lateness` 只有一个数；现在它被拆成两侧，且这条恒等式对每份证据都成立：

```
lateness[k] − lateness[k−1] = present_step[k] − due_step[k]
present_step[k] = (present[k] − present[k−1]) − frame_duration   ← 已有的 present_jitter
due_step[k]     =  due[k] − (due[k−1] + frame_duration)          ← 本卡新增
```

最差帧连同它的 `due_step`、前 2 帧、后 8 帧一起出版，于是「手慢了」与「表快了」
只剩一道减法，不再靠读位置去猜。

---

## 二、真机 13 轮（`docs/evidence/p13p/`，schema `kivo.p12c.sync/14`）

片源 `p13c_4k60_av.mp4`（8 s / 480 帧），`cpu_paced`，零丢帧零 xrun。

| 轮 | max (ms) | p99 (ms) | `due_step` (µs) | `present_step` (ms) | 判定 | 位置比 |
|---|---|---|---|---|---|---|
| r1 | 3.099 | 0.625 | +24 | +3.050 | flat | 0.74 |
| r2 | 1.996 | 0.125 | −2 | +1.921 | flat | 0.91 |
| r3 | 5.566 | 0.875 | −19 | +5.466 | flat | 0.50 |
| r4 | 6.545 | 0.875 | +36 | +6.348 | flat | 0.03 |
| r5 | 9.592 | 1.375 | −15 | +9.274 | flat | 0.01 |
| r6 | 6.318 | 1.125 | −6 | +6.217 | flat | 0.08 |
| r7 | 11.796 | 1.625 | +7 | +11.729 | flat | 0.40 |
| **r8** | **45.636** | 2.375 | **−21** | **+45.372** | **`present_side`** | 0.76 |
| r9 | 6.445 | 1.875 | +8 | +6.382 | flat | 0.82 |
| r10 | 5.766 | 1.375 | −11 | +5.685 | flat | 0.03 |
| r11 | 6.460 | 0.875 | −11 | +6.349 | flat | 1.00 |
| r12 | 9.486 | 1.875 | −2 | +9.397 | flat | 0.71 |
| **对照** | **79.856** | 5.375 | **+2** | **+79.777** | **`present_side`** | 0.43 |

**两列一眼能读出来的事**：`due_step` 一列全是微秒级（最大 36 µs = 0.2% 帧长），
`present_step` 一列与 `max` 几乎逐位相等。**这一轮的设备证据里没有"表快了"。**

12 轮的 max 中位数 **6.45 ms**（P13-F 的十轮是 4.0 ms，同一台机器、同一片源，
差在一次构建），11/12 轮 ≤ 11.8 ms。

### 2.1 唯一一次真尖峰：r8

`lateness` 47 → 45.6 ms，`due_step = −21 µs`（目标时刻没动），`present_step = +45.372 ms`
（**这一帧确实晚交了 45 ms**）。尾巴 `[29.73, 13.65, −1.11, 0.07]` —— 每帧掉约 16 ms，
正好一帧一步排空，是"一次延迟 + 随后连交三帧"的形状，不是孤立样本。
`over_budget_frames = 2`，`frames_dropped = 0`。

### 2.2 正向对照：停 60 ms

`SIGSTOP` / `SIGCONT` 打在 4 s 处（实测迟到 79.856 ms，比请求的 60 ms 多约 20 ms，
多出来的是两次 `adb shell` 往返与 `kill` 本身）：

| 量 | 值 | 读法 |
|---|---|---|
| `lateness_ns` | 79.856 ms | 停多久就迟多久 |
| `due_step_ns` | **+2 µs** | 目标时刻完全没动 |
| `present_step` | **+79.777 ms** | 全部在"手慢了"这一侧 |
| 尾巴 | `[11.27, 0.07, 0.07, 0.10]` | 一帧就回到基线 |

判定 `present_side` —— **仪器能分辨两个方向**，不是个只会说 `due_side` 的哑指标。

---

## 三、设计稿 §1.2 被推翻（本卡最重要的一条）

设计稿当时写的是：

> 实测整场 `present_step` 的最大值只有 19.586 ms ⇒ 这 88.231 ms 不可能来自"手慢了"，
> 剩下的量只能在 `due` 侧。

**真机上 due 侧一次都没发生。** 于是回头找那条推理的漏洞，漏洞在**首帧没有前一帧**：

- `present_jitter` 的 479 个样本来自第 1..479 帧，**第 0 帧的交出延迟根本不进直方图**；
- `p13d-4k60-r1` 的 `first_frame_latency_us = 263882`（264 ms，是今天 12 轮的 2.5–4 倍）；
- `present_jitter.min = −16.317`（正好一帧的排空量）；
- `over_budget_frames = 6`，88 → 71 → 55 → 38 → 21 → 5 正好 5–6 帧。

四条都与"**开头第一帧晚了 88 ms，随后一帧一步排空**"吻合 —— 而那是 `present` 侧。
被直方图排除的只是"片中单帧 88 ms"，不是"首帧 88 ms"。

所以 P13-F 的「片中抢占」既不成立也不该保留。但**本文第一版的替换结论「首帧排空」同样是错的**，
写完之后核了一遍 `FramePacer::arrive()`：首帧建锚点时 `due_at` 就取 `reading.mono_ns`，
`lateness = mono − due ≡ 0` —— **第 0 帧不可能迟到 88 ms**。那段更正因此作废，
留在这里是因为它演示了同一类错误：拿一条"看起来吻合"的算术去替代观测。

真正剩下的解释只有一个方向：**`due` 整条时间轴被推早，然后几帧内收敛**。
它每一步只挪一帧左右（`due_step` 的绝对值 ≤ 16.667 ms），所以 `due_step_frames`
（门限是一帧）会记 0，而 `lateness` 在这几帧里从 88 一路掉到 5。
这条**单帧快照看不出来**，要看两侧的**累积**：`P13-Q` 就是补这个。
`/2` 时代的证据没有 `worst` 字段，位置永远不会知道了 —— 这一点不变。

> 这条更正留在设计稿原文下方（引用块），不删原文：
> 一个被真机推翻的推理，比一个从来没写下来的推理有用。

---

## 四、判据与单测

- `kivo_p13p_episode_tests`：**8 例 / 57 断言**（`t01`–`t08`），
  重点在三条"会把未知伪装成已知"的路：没有最差时不给邻帧、新最差丢掉旧尾巴、
  不带 `DueStep` 时 `due_step_supplied` 必须为 0。
- `ctest -R "kivo_p13|kivo_p12c5"`：**15/15**
- `python tools/verify_p13p_episode.py --self-test`：正例 1 + 变异 7，**8/8**
- `python tools/verify_p13p_episode.py docs/evidence/p13p/*-sync.json`：
  **13 份复算通过**，归类 `flat=11` / `present_side=2`
- 总收口 `python tools/verify_p12_all.py`：**416 份 / 四层全通过 / 9 个子校验器 exit 0**

---

## 五、诚实清单

1. **88 ms 仍然没有被直接测到。** 今天的 12 轮最大 11.796 ms；45.636 ms 那次是另一件事
   （它在片中，位置比 0.76）。§三 的"首帧排空"是**唯一与全部四条旧数据相容的解释**，
   不是观测。要证实它，需要一次 `worst.index < 6` 且 `max > 40 ms` 的证据 —— 没撞上。
2. **`due_step` 一路 0 不等于"时钟永远准"。** 它只覆盖"目标时刻相对上一帧偏了多少"，
   覆盖不到"锚点整体建错"（那会让整场的 `lateness` 一起平移，`due_step` 全是 0）。
   后者要靠 `av_drift` 与 `present_jitter` 的绝对值，本卡没碰。
3. **`present_step` 是反推出来的，不是直接记录的。** 证据里只有 `due_step`、
   `lateness` 与前一帧的 `lateness`；`present_step = Δ + due_step`。
   它与 `present_jitter` 的直方图端点一致（校验器 P4 守这条，容差半个桶宽 0.125 ms），
   但端点一致只是必要条件 —— 若两侧同时错、且错得对称，P4 看不出来。
4. **正向对照的 60 ms 是请求值，不是实测值。** 实测 79.856 ms，多出的 ~20 ms 是
   `adb shell` 两次往返 + `kill` 自身。**不能拿 79.856 去反推"停了 80 ms"**。
5. **12 轮里有一轮的 `max` 是中位数的 7 倍（r8）。** 这就是这张卡的题目本身，
   但它也说明"12 轮不够"：88 ms 那种量级按今天 1/13 的尖峰率，撞上要走运。
6. **`r8` 的成因没有归到具体机制。** `present_side` 只说明"交出晚了 45 ms"，
   分不出是线程被抢占、解码与呈现同线程（`pipeline_mode = merged`）被一帧解码拖住、
   还是 `releaseOutputBuffer` 本身慢。要分开得记每帧的解码/提交耗时，本卡没做。
7. **两张旧卡的红灯是本卡发现的，但成因在它们身上**：`p13n` 的 `sizeof(DriftTrend) ≤ 448`
   从 P13-O 加进 `HalvesAgreement` 那天起就是红的（实测 528）；`p13l` 的两条预算断言
   被 P13-O 的"忽略浮点噪声级差异"缺失打红（完美线性序列被判 `inconsistent` → `unknown`）。
   两处都已修，并且**修的是根因**（`kMinCumulativeGapNs`），不是把断言改松。
8. **只测了 4K60 一档片源、一台设备。** 1080p60 没跑 —— 按 §三 的解释（首帧开销），
   1080p 的首帧应该更快、更不容易撞上，但这只是推论。

---

## 六、产物

- 设计：`86-p13p-design.md`（含 §1.2 的更正块）
- 新增：`core/pace/pace_worst_episode.{hpp,cxx}`（最差帧 + 前后邻帧，零分配）、
  `tests/video/pace/p13p_episode_tests/episode_tests.cpp`、`tools/verify_p13p_episode.py`
- 改：`pace_action`（`PaceDecision::due_step_ns`）、`frame_pacer`（算 `due_step`）、
  `pace_stats`（`DueStep` 入参、`due_step_frames` / `due_step_supplied`、
  `frame_worst` 改为由 `frame_episode` 派生）、
  `av_sync_report`（`sync/14` + `worst` 扩三格）、
  `pace_halves_agreement`（`kMinCumulativeGapNs`）、
  `tools/verify_p12_all.py`（登记 `/14` 与第 9 个子校验器）、
  `p13n_quantile_tests`（体积断言 448 → 528）、`p13n_quantile_tests` 之外的
  `p13o_halves_tests`（新增 O13）
- 证据：`docs/evidence/p13p/`（13 轮 × decode + sync = 26 份）

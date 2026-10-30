# 86 · P13-P 设计：迟到尖峰要分成「手慢了」和「表快了」

> 卡片：P13-P
> 前置：P13-F（`66-p13f-design.md` 给出最差点的**位置**）、P13-D（抽稀口径）、P13-M（时钟降级）
> 结论：把 `lateness` 拆成 `present` 侧与 `due` 侧两个分量，用一条恒等式在证据里**可复算**；
> 据此对 P13-E 遗留的 88.231 ms 做回溯归因，并撤回 P13-F 的「片中抢占」措辞
> schema：`kivo.p12c.sync/13` → `/14`

---

## 一、问题

### 1.1 「最差在哪」回答了，但「最差是什么」没回答

P13-F 给了 `worst = { index, of, media_ns, lateness_ns }`，于是知道最差点落在片头还是片尾。
十轮实测（`docs/evidence/p13f/`）位置比 `0.19 0.06 0.32 0.67 0.06 0.58 0.14 0.30 0.80 0.00`，
于是写下结论：**片中抢占，与 4K 分辨率无关**。

这个推理有一处跳步：**位置分散只能排除"固定发生在片头/片尾"，排不出"它根本不是渲染侧的事"**。
一段时钟读数的跳变也会落在任意位置 —— 它跟位置同样无关，但原因完全不同：

| 若是 | 机制 | 该改哪里 | 4K 是不是变量 |
|---|---|---|---|
| 渲染线程被抢占 | 交出动作本身晚了 | 线程优先级 / 解码与呈现同线程（`merged`） | 是（4K 更重） |
| 时钟侧位移 | 目标时刻被算早了，画面按时交，但"按时"的基准错了 | `ClockPositionTracker` / 锚点映射 | **否** |

两种解释在现有证据里长得一模一样：都只让 `lateness` 变大。

### 1.2 但 P13-E 那份 88.231 ms 已经能被算术判掉一半

`lateness` 与 `present_jitter` 不是两个独立指标，它们逐帧满足一条恒等式：

```
lateness[k] − lateness[k−1] = present_step[k] − due_step[k]
其中  present_step[k] = (present[k] − present[k−1]) − frame_duration   ← 就是 present_jitter 的样本
      due_step[k]     =  due[k] − (due[k−1] + frame_duration)          ← 目标时刻偏离"应然"多少
```

于是 `present_jitter` 的直方图**给 `present` 侧封了顶**。`p13d-4k60-r1` 里：

| 量 | 值 |
|---|---|
| `pace.lateness_ns.max` | **88.231 ms** |
| `present_jitter.max` | **19.586 ms** |
| `present_jitter.min` | −16.317 ms |
| `pace.over_budget_frames` | **6** |
| `frame_duration_ns` | 16.667 ms |

一帧若真被拖了 88 ms，`present_step` 至少得是 +71.5 ms（88 − 16.7）。
实测整场 `present_step` 的最大值只有 19.586 ms ⇒ **这 88.231 ms 不可能是"某一帧被拖住了"**。

⚠️ 这里要说准：不能由此断言"某一帧的 `due_step` 就是 −68.6 ms"。
位移可以分几帧累积 —— 那 6 个超预算帧每帧偏 15 ms，也能堆出 88 ms。
能确定的只有一件事：**没有任何一帧交出得比 19.586 ms 更晚**，所以"手慢了"这个解释
在单帧尺度上被排除了；剩下的量只能在 `due` 侧。

> **⚠️ 本段在 P13-P 真机上被推翻，保留原文以便对读（更正见 `87-p13p-status.md` §三）。**
>
> 真机 13 轮（12 轮 4K60 + 1 轮正向对照）里 `|due_step| ≤ 36 µs`，一次 due 侧位移都没有；
> 反而是 45.636 ms 的那一轮干净地落在 `present_side`（`due_step = −21 µs`，
> `present_step = +45.372 ms`）。也就是说"目标时刻被算早"这条路径在本机**没发生过**。
>
> 上一段推理的漏洞在**首帧没有前一帧**：`present_jitter` 的 479 个样本来自第 1..479 帧，
> 第 0 帧的交出延迟**不进直方图**。而 `p13d-4k60-r1` 的 `first_frame_latency_us = 263882`
> （264 ms，是其它轮的 2.5–4 倍）、`present_jitter.min = −16.317`（正好一帧的排空）、
> `over_budget_frames = 6`（88 → 71 → 55 → 38 → 21 → 5，约 5–6 帧）——
> 三条都与"**开头第一帧晚了 88 ms，随后按一帧一步排空**"吻合，而那正是 `present` 侧。
> 单帧 88 ms 的"片中"抢占被直方图排除，但"首帧 88 ms"没有。

同一条恒等式还说明这不是"孤立一个样本"：`over_budget_frames = 6` 是 6 个超过一帧预算的样本，
而 `present_step` 全程都在 ±20 ms 内 ⇒ 那 6 帧的 `due` 被连续推早了，随后又回锁。
这正是"时钟读数跳了一下、几帧后收敛"的形状，不是"线程卡了 88 ms"（那会先给一个 +71 ms 的
`present_step`，再给一串递减的迟到）。

### 1.3 为什么现在才看出来

因为 `lateness` 与 `present_jitter` 在报告里隔着一百多行，且**没有任何字段说它们共享同一批帧**。
P13-F 拿 `index/of` 看位置，没有拿 `present_jitter` 去夹 `lateness` —— 两个数各自都"合理"。

字段分居两处时，读者只会分别读；**能让人算的，才会被算**。所以这张卡不是"再分析一次"，
是把 `due_step` 出版，让这条恒等式对每一份证据都成立、可复算。

---

## 二、判据设计

### 2.1 三个新数（都在最差帧上）

| 字段 | 含义 | 为什么必须出版 |
|---|---|---|
| `worst.due_step_ns` | 该帧 `due` 相对"上一帧 `due` + 一帧"的偏差 | 这是恒等式里唯一缺的那一项；有了它，`present_step` 可以直接解出来 |
| `worst.prev_lateness_ns[]` | 最差帧之前 2 帧的 `lateness` | 恒等式要 `lateness[k−1]`；只给 `max` 时无从下手 |
| `worst.next_lateness_ns[]` | 之后 8 帧的 `lateness` | 区分"一步跳变"（立刻回到基线）与"持续位移"（慢慢收敛） |

`pace.due_step_frames`：整场 `|due_step| > frame_duration` 的帧数。它是这张卡的
**一行摘要**：非零即说明这一轮的"时间基准"动过，与渲染快慢无关。

### 2.2 三态判定（写进校验器，不靠人读）

对最差帧，令 `Δ = lateness[k] − lateness[k−1]`（`prev` 为空时不判）：

| 关系 | 判定 | 读法 |
|---|---|---|
| `|due_step| ≥ 0.75·|Δ|` 且 `|Δ| > 一帧` | `due_side` | 表快了：目标时刻被算早 |
| `|Δ − due_step| ≥ 0.75·|Δ|` 且 `|Δ| > 一帧` | `present_side` | 手慢了：交出动作本身晚 |
| 其余 | `mixed` | 两侧都有份，不给单一结论 |

0.75 是**归档阈值**，不是物理常数：它只用来把"主因"挑出来，`mixed` 是合法结果，
不许为了凑一个干净结论去调它。判定结果同时要求 `|Δ| > frame_duration`，
否则那点差本来就在噪声里（真机 p50 = 0.125 ms）。

### 2.3 正向对照：必须能看见"手慢了"

对 `due_side` 的判定要有反证：**故意让渲染线程停 60 ms**（`SIGSTOP` / `SIGCONT`，
P12-D5 用过的同一手法），此时 `present_step ≈ +60 ms`、`due_step ≈ 0`，
校验器必须判 `present_side`。这条对照证明"分成两侧"这件事有分辨力，
而不是一个永远只会说 `due_side` 的哑指标。

> 对照只证明**仪器能分辨**，不证明 88 ms 的成因。后者靠 2.1 的回溯 + 复现轮次。

---

## 三、自然家族树

| 文件 | 职责 |
|---|---|
| `core/pace/pace_worst_episode.{hpp,cxx}` | 最差帧 + 前后邻帧的记录器（纯数据，无 IO） |
| `core/pace/pace_action.hpp` | `PaceDecision` 新增 `due_step_ns`（节拍器算，不在这里算） |
| `core/pace/frame_pacer.cxx` | `arrive()` 里算 `due_at − (last_due + T)` |
| `core/pace/pace_stats.cxx` | 逐帧喂记录器；累计 `due_step_frames` |
| `backends/avsync/report/av_sync_report.cxx` | `sync/14`，扩展 `worst` 块 |
| `tools/verify_p13p_episode.py` | 恒等式、三态、`due_step_frames` 复算 |

`pace_worst_episode` 单独成文件而不是塞进 `pace_stats`：它有一套自己的状态机
（当前最差、前 2 帧环、后 8 帧填充），塞进去会让 `PaceStats` 从"计数器"变成"两个东西"。

---

## 四、字段清单（`sync/14` 的增量）

```json
"pace": {
  "due_step_frames": 6,
  "lateness_ns": {
    "worst": {
      "index": 211, "of": 480, "media_ns": 3518667000, "lateness_ns": 88231396,
      "due_step_ns": -71234567,
      "prev_lateness_ns": [125000, 208333],
      "next_lateness_ns": [71666667, 55000000, 38333334, 21666667, 5000000, 125000, 125000, 125000]
    }
  }
}
```

- `due_step_ns`：**有符号**。负 = 目标时刻被算早了（表快了），正 = 被算晚了。
- `prev_*` / `next_*`：长度可变，末尾不足就是没采到（会话结束），不补零。
- `worst` 块的位置不变（仍在 `max` 与 `histogram` 之间），`histogram` 仍是最后一个成员。

---

## 五、不做什么

| 不做 | 理由 |
|---|---|
| 不在报告层重算 `due_step` | 那里没有 `last_due`，重算等于用 `Δlateness + interval` 反推 —— 反推出来的数无法与"节拍器当时看到的"对账 |
| 不给 `present_step` 单独开直方图 | 它就是已有的 `present_jitter`，再存一份 = 同一笔账记两处 |
| 不因为 `due_side` 就立刻改时钟代码 | 先要把"跳变"复现出来（12 轮 + 对照）；改在没复现的地方等于蒙 |
| 不用 `SIGSTOP` 的 60 ms 去解释 88 ms | 对照只验仪器，不构成成因证据 |

---

## 六、验收

| 编号 | 判据 |
|---|---|
| P1 | 单测：`due_step` 的三种形状（0 / 负跳 / 正跳）各自落到 `present_side` / `due_side` |
| P2 | 恒等式 `Δlateness = present_step − due_step` 在真机证据上逐位成立（校验器复算） |
| P3 | `next_lateness_ns` 长度 = `min(8, of − 1 − index)`；越界不写 |
| P4 | 正向对照（`SIGSTOP` 60 ms）判 `present_side`，且 `present_step ≥ 50 ms` |
| P5 | 12 轮 4K60：`due_step_frames` 与 `max` 的联合分布能回答"88 ms 是否复现" |
| P6 | `ctest -R kivo_p13` 全绿；总收口 `verify_p12_all.py` 四层通过 |

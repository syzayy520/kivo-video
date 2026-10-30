# 88 · P13-Q 设计：账要按整段算，不能只看最差那一帧

> 卡片：P13-Q
> 前置：P13-P（`86-p13p-design.md` 的 `due_step` 拆单帧）、P13-O（半程比较）
> 结论：把两侧的**累积**出版，跨帧恒等式 `Δlateness = Σpresent_step − Σdue_step` 可复算；
> 单帧看不见的"每步都小、累积很大"的位移因此现形
> schema：`kivo.p12c.sync/14` → `/15`

---

## 一、问题

### 1.1 P13-P 的尺子只量得出"一步"

P13-P 把最差帧拆成 `due_step`（目标时刻偏了多少）与 `present_step`（交出晚没晚），
判据是**单帧差分**：

```
lateness[k] − lateness[k−1] = present_step[k] − due_step[k]
```

于是 `due_step_frames` 的门限只能是"超过一帧"。可一个**缓慢的**位移每步只挪半帧、
连着挪十帧，累积 80 ms —— 每一步都不触发门限，计数记 0，看起来"目标时刻纹丝没动"。

这不是假设。`p13d-4k60-r1` 的 88.231 ms 配着一串 6 帧的迟到（`over_budget_frames = 6`），
形如 88 → 71 → 55 → 38 → 21 → 5：每帧掉一帧左右。单帧看，每次只掉 16.7 ms；
整段看，是 83 ms 的位移。

### 1.2 首帧不可能是那一帧

写 P13-P 结题时我一度把 88 ms 归到"首帧交出慢"。核代码后作废：
`FramePacer::arrive()` 在首帧建锚点，`due_at` 直接取 `reading.mono_ns`，
`lateness = mono − due` **恒等于 0**。第 0 帧不可能迟到 88 ms。
（更正记录见 `87-p13p-status.md` §三。）

剩下唯一与全部旧数据相容的形状，就是 §1.1 那个"缓慢位移"。
要测它，尺子必须从"差分"换成"累积"。

---

## 二、判据设计

### 2.1 一条跨帧恒等式

对每个被呈现的帧记两个步长，各自**独立测量**：

| 量 | 来源 | 含义 |
|---|---|---|
| `present_step` | 交出时刻的时间戳（就是 `present_jitter` 的样本） | 这一帧交得比节奏早/晚多少 |
| `due_step` | 时钟算出的目标时刻 | 目标时刻比"上一帧 + 一帧"偏了多少 |

两者累加，满足（逐项相加，无近似）：

```
lateness[k] − lateness[0] = Σ present_step − Σ due_step
```

`lateness[0] = 0` 是锚点带来的不变量（§1.2），出版它是为了让人**当场复算**这条式子，
而不是去翻 `frame_pacer.cxx` 才知道右边常数是 0。

### 2.2 出版什么

`pace` 块五格（整场）+ `worst` 块三格（到最差帧为止）：

```json
"pace": {
  "first_lateness_ns": 0, "last_lateness_ns": 40491234,
  "sum_present_step_ns": 12083333, "sum_due_step_ns": -28432901,
  "closure_gap_ns": 0
},
"worst": { "cumulative_present_step_ns": 12083333, "cumulative_due_step_ns": -28432901,
           "lateness_at_first_ns": 0 }
```

**`closure_gap_ns` 是残差，不是断言。** 中间有帧没被记账（丢帧、锚点重建）时它必然非 0，
那时这套账不可用 —— 出版残差，比"假装它一定是 0"诚实，也让校验器有一条可判的线。

### 2.3 已知限制：丢帧会让账不平

`note_presented()` 只对被**呈现**的帧调用，而跨过丢帧时 `present_step` 用的是
"帧数 × 帧长"（`frames_since_present_`），`due_step` 却只记了最后一帧。
两边的分母不同，账就不平 —— 实测差 140 ms（`p13q-4k60-r1` / `-r5`，各丢 2 帧）。

本卡**不修**这条：要修得让丢帧也进账（`arrive()` 里累加全部 `due_step`），
那会改变 `due_step_frames` 的口径，属于另一张卡。现在做的是：
校验器只在 `frames_dropped == 0` 时要求闭合，丢帧轮只核对残差自身自洽。

---

## 三、字段与落位

| 文件 | 改动 |
|---|---|
| `core/pace/pace_worst_episode.hpp` | `WorstEpisode` +3 格（两个累积量、首帧 lateness） |
| `core/pace/pace_stats.hpp` | `DueStep` +`present_step_ns`；`PaceSummary` +5 格 |
| `core/pace/pace_stats.cxx` | 两侧累加；残差 = `(last − first) − (Σpresent − Σdue)` |
| `core/pace/frame_pacer.cxx` | `present_step` 与 `note_interval()` 共用一次计算 |
| `backends/avsync/report/av_sync_report.cxx` | `sync/15`，`pace` 五格 + `worst` 三格 |
| `tools/verify_p13p_episode.py` | Q1–Q3（整场闭合 / 最差帧闭合 / 首帧不变量） |

---

## 四、不做什么

| 不做 | 理由 |
|---|---|
| 不把 `due_step_frames` 的门限调小 | 门限一帧是为了对齐 G6 的 `over_budget`；调小会把噪声计成位移。累积量才是这个问题的正确尺子 |
| 不用累积量替代单帧判定 | 单帧回答"这一步是谁的"，累积回答"整段是谁的"。两者一起读才有意义 |
| 不在丢帧轮强行把账做平 | 那需要改丢帧路径的记账口径，会把 `due_step_frames` 的含义一起改掉 |

---

## 五、验收

| 编号 | 判据 |
|---|---|
| Q1 | `(last − first) == Σpresent − Σdue`；残差与复算一致；零丢帧时残差 ≤ 1 µs |
| Q2 | 最差帧：`lateness − lateness_at_first == cum_present − cum_due` |
| Q3 | `first_lateness_ns == 0`（锚点不变量）；非 0 即整条账的前提失效 |
| Q4 | 单测：跨帧账闭合、残差被如实出版 |
| Q5 | 真机：空载轮残差 0；满载轮能看到 due 侧累积位移 |

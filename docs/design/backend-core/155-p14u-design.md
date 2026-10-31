# P14-U 设计卡 · 报告说差 125 µs，耳朵听到的是 427 ms

> 一张卡只答一个问题：**`av_drift` 该相对谁？**
> 答案：**相对墙，不相对这只钟。** 因为这只钟能在 8 秒里谎报 5.36%，
> 而墙钟不会 —— 两者差两个数量级。这一卡把"相对墙"的那一份并排印出来，
> 并给它一个**可算的误差上界**，让它从"仅供参考"变成能写进结论的数。

---

## 一、起因

P14-T 关掉了"4 份越界成因"这个问题，并在 §十.1 留下下一张卡：

> **`av_drift` 该相对谁。** 现在它相对"这只钟"，而听得见的是"相对 DAC 实际播出的音频"。
> 本卡证明两者能差 427 ms。要做的是让 `av_drift` 也相对"装置侧"报一份，
> 或者至少把领先量扣掉再报一份 —— **两个数并排，才谈得上"音画偏差是多少"**。

那一卡已经证明了两件本卡要用的东西，直接引用、不再重证：

1. `clock_lead_ns = elapsed_span − arrive_mono_span`，四份逐份对上"墙钟比实测跨度短"，
   差都在一帧以内；110 份带实测跨度的证据逐份复算一份不差。
2. 领先量是**每一轮各自的偏移量**，不是速率：10 份 8 秒片源的墙钟从 7.94 缩到 7.56 s，
   领先量从 46 散到 427 ms，媒体跨度全是 7.983 s。同一段媒体被压进不同长度的墙钟，
   压多少就是领先多少。

## 二、这一卡要回答的三个问题

### 2.1 第二份怎么算（纯派生，不改任何测量）

`av_drift` 现在是 `L_k = present_k − due_k`，而 `due_k = mono_k + (pts_k − anchor_pts − elapsed_k)`。
把 `present_k` 换成"墙钟按 pts 排布时它该出现的时刻" `W0_k = mono_0 + (pts_k − anchor_pts)`：

```
W_k = present_k − W0_k
    = [mono_k + pts_k − anchor_pts − elapsed_k + L_k] − [mono_0 + pts_k − anchor_pts]
    = L_k − (elapsed_k − (mono_k − mono_0))
    = L_k − lead_k
```

`lead_k = elapsed_k − (mono_k − mono_0)` 就是"这一帧上，钟比墙多走了多少"。
于是**第二份 = 第一份减去 leading**，逐帧、零新测量、只用 `DueStep` 里已有的
`clock_mono_ns` / `clock_elapsed_ns` 与 `lateness_ns`。

> 这不是"两个已出版的数相减"（P14-L 反的那类）。`L_k` 与 `lead_k` 是两条独立链：
> 前者来自"交出时刻 − 目标时刻"，后者来自"钟的位置 − 墙的位置"。它们只共用 `due` 的定义式。

### 2.2 第二份能不能读作音画偏差

**能，但只能以"估计"的身份，且必须带误差上界。** 三步论证：

1. **耳朵的参考系是 DAC，不是这只钟。** DAC 的推进由音频晶振决定，与 `CLOCK_MONOTONIC`
   同属 SoC 时基，消费级晶振频差在 ±50 ppm 量级。
2. **`lead` 不可能是频差。** ±50 ppm 在 8 秒上只累积 0.4 ms；实测领先量散在
   8 µs – 636 ms，跨五个数量级。所以 `lead` 是**位置偏移**（钟报的位置不是真正播到的位置），
   扣掉它之后的 `vs_wall` 与"DAC 实际播出位置"只差一个**固定延迟**（一帧量级）。
3. **那个固定延迟在锚点处已被吸收。** `anchor_audio_position_ns` 记的就是建锚时刻的音频位置，
   而 `av_drift` 本身就是相对锚点的相对量（报告里 `anchor.note` 已写明）。

所以 `vs_wall` 的误差上界取 `max(一帧, 50 ppm × 窗口)`：
8 秒窗口上一帧（16.7 / 33.3 ms）远大于 0.4 ms，**上界就是一帧**。
这个数不写死在文档里，出版成 `bound_ns` 一格，算法跟着字段走。

> ⚠️ **命名是这一节最要紧的决定：它叫 `vs_wall`，不叫 `vs_audio`。**
> 我们**没有**测到 DAC 真正播到哪一帧 —— 项目里没有第二个时钟源。
> 叫 `vs_audio` 就是把"相对墙的估计"冒充成"相对耳朵的测量"，
> 而这正是 P14-T 抓到的那个病：一个名字说得比它做的多，读者就再也不会去查。

### 2.3 为什么历史那 427 ms 不可能是速率（把推论升级成逐份判据）

P14-T §三 的证据是**跨份对比**：8 秒窗口一样长，领先量却散 10 倍 → 只可能是各轮各自的偏移。
这条论证成立，但它需要"凑一批"。这一卡把它变成**单份可判**：

```
lead_ppm = |clock_lead_ns| × 1e9 / (arrive_mono_span_ns × 1e6)
         （即 |lead| / 墙钟跨度，单位 ppm）
```

`lead_ppm > kRateExplanationUpperPpm = 2000` 时，"这一段是频差"在物理上不成立 ——
2000 ppm 已经是消费级晶振典型值的 40 倍，留了足够余量。
此时报告必须能说出"这是位置偏移"，而不是让读者自己去推。

全仓 113 份带钟读数的证据里，`lead_ppm > 2000` 的有 **11 份**（最小 5857 ppm），
所以这 11 份**一律**判"必非速率"；其余 102 份落在这个上限内，**不许**被标成偏移。

## 三、家族树（新文件放哪）

```
backend/include_private/video/video_plane/core/pace/
  pace_lead_trace.hpp        ← 新：钟相对墙的逐帧领先量 + 阶跃检测（一个职责）
  pace_wall_offset.hpp       ← 新：视频相对墙的偏移摘要（另一个职责）
backend/src/video/video_plane/core/pace/
  pace_lead_trace.cxx
  pace_wall_offset.cxx
backend/tests/video/pace/p14u_lead_trace_tests/
  lead_trace_tests.cpp       ← 新：五个场景
```

不放进 `pace_stats.hpp`：那个文件已经 442 行（铁律 §六.3 的 400 行线就在眼前），
再塞两个类就是堆砌。也不合成一个 `pace_reference_shift`：
"钟偏多少"与"视频偏多少"是两个问题，合在一起会有两个写入口。

## 四、改法

### 4.1 `LeadTrace`（`core/pace/pace_lead_trace.hpp`）

单入口 `note(elapsed_ns, mono_ns)`，首帧自动立基准（`elapsed_0` / `mono_0`），
每帧算 `lead_k` 并 O(1) 维护：

| 字段 | 意思 |
|---|---|
| `samples` | 喂进来的帧数（0 或 1 时全块无意义） |
| `first_ns` / `last_ns` | 首帧之后第一帧的领先量 / 末帧领先量 |
| `max_ns` / `min_ns` | 全程极值 |
| `max_step_ns` | **相邻帧**领先量的最大绝对变化 |
| `step_frames` | `|Δlead| > frame_duration` 的帧数 |
| `frame_duration_ns` | 判"这是不是一步"的尺子（调用方给） |

`verdict()` 四态，**不是二三态** —— 因为"没量到"和"很稳"必须分开（P14-L 的老病）：

| verdict | 条件 | 读法 |
|---|---|---|
| `unknown` | `samples < 2` 或没给帧长 | 这一块什么都不许读 |
| `stepped` | `step_frames > 0` | 钟**跳**过，`max_step_ns` 就是台阶大小 |
| `ramped` | 未跳，且 `|last| > frame_duration` | 钟在**匀速**跑偏，`last/span` 是 ppm |
| `steady` | 未跳，且 `|last| ≤ frame_duration` | 领先量在一帧内，`av_drift` 可读作音画偏差 |

`verdict_reason` 每次都非空（G9 的同一条要求用在派生量上）。

### 4.2 `WallOffset`（`core/pace/pace_wall_offset.hpp`）

单入口 `note(offset_ns, media_ns)`，维护 `samples` / `first_ns` / `last_ns` /
`worst_ns`（`|offset|` 最大）/ `worst_media_ns` / `worst_index`。

**不建直方图。** 理由：`offset = L − lead`，而 `lead` 是缓变或阶跃，
分布形状几乎就是 `lateness` 的形状平移；再养一份 1024 桶直方图 + 抽稀序列 + 趋势回归，
是为一个已经能由两个数算出来的东西付三份代价（`pace_stats.hpp` 里 P12-C8 那段注释记的就是这个判断）。
极值 + 首末三格足够回答"差多少、在哪儿"。

### 4.3 接线（`pace_stats`）

`PaceStats::note_presented()` 在**已有**的钟读数分支里加两行：

```cpp
const auto lead = lead_trace_.note(step.clock_elapsed_ns, step.clock_mono_ns);
wall_offset_.note(lateness_ns - lead.current_ns, media_ns);
```

`lead_trace_` 只收 `step.clock_mono_ns != 0` 的帧（与 `clock_supplied_frames` 同一个门），
否则 `surface_at_time` 档会喂进 480 个 0 并把 `verdict` 判成 `steady` —— 那是假观测。

`PaceSummary` 加三格：`LeadTrace lead_trace`、`WallOffset wall_offset`、
`wall_reference_bound_ns`。

**同一个改动顺带修了一处既有缺陷**（写在这里是因为它改变了判据的强度）：
四个跨度原先的**基准帧**取在"有没有钟读数"的判断**之外** —— `has_span_` 在第一帧就立，
哪怕那一帧的 `clock_mono_ns` 是 0。于是 `surface_at_time` 或任何"第一帧还没钟"的轮次，
`clock_lead_ns` 会变成 `last_elapsed − last_mono`（凭空多出几秒），
而两把尺子的基准帧也不再是同一帧。
把整段挪进分支之后，**全帧有读数的轮次逐位不变**（每帧都进那个分支），
U1 因此可以从"差一帧以内"收紧到"逐位相等"。

### 4.4 报告层（`sync/32`）

`pace` 块加 `clock_lead_trace`（13 格，含 `verdict` / `verdict_reason` / `rate_ppm`）；
`av_drift` 块加 `reference: "audio_clock"` 与 `vs_wall`（9 格，含 `bound_ns` / `measured` / `reason`）。

`pace.clock_lead_ns` **一个字不改** —— P14-T 的 110 份判据在读它，改结构等于让那批判据全部失明。

### 4.5 判据层

`tools/verify_p12c_sync.py` 新增 U 组：

| 判据 | 内容 |
|---|---|
| U1 | `clock_lead_trace.last_ns` **逐位等于** `clock_lead_ns`（两把尺子同源自证；为此把四跨度的基准帧挪进了"有钟读数"的分支，见 §4.3） |
| U2 | `vs_wall.last_ns == last_lateness_ns − clock_lead_trace.last_ns`（**逐位**，这是定义式） |
| U3 | `clock_supplied_frames == 0` 时 `verdict == "unknown"` 且 `vs_wall.measured == false` |
| U4 | `rate_ppm > 2000` 时 `verdict != "ramped"`（"5% 是速率"这种读法必须不可能） |
| U5 | `verdict == "stepped"` 时 `step_frames ≥ 1` 且 `|max_step_ns| > frame_duration_ns` |

`tools/audit_clock_lead_origin.py`（新，离线）：对全仓 `*-sync.json` 逐份算 `lead_ppm`，
输出 `必非速率 / 上限内` 两栏，**带反向判据**（某份从"必非速率"掉回上限内也要红）。

## 五、预言（真机核对，写在这里是为了能被推翻）

| 组 | 参数 | 预期 |
|---|---|---|
| A 阶跃 | `clock_offset_ms=400`、门关、cpu_paced | `verdict=stepped`、`max_step≈400 ms`、`vs_wall.last≈−400 ms`、`av_drift.p50` 仍 ≈125 µs |
| B 速率 | `clock_rate_ppm=5000` | `verdict=ramped`、`last≈40 ms`、`step_frames=0` |
| C 对照 | 无注入 | `verdict=steady`、`|vs_wall.last| ≤ 一帧` |

A 组是本卡的关键：**它能在一份证据里同时显示"`av_drift` 说 125 µs"和"`vs_wall` 说 −400 ms"**，
而这两句话都出自同一批帧。这就是"报告在撒谎"的活体标本。

## 六、不做的事

| 不做 | 原因 |
|---|---|
| 叫它 `vs_audio` / `audio_video_offset` | 我们没测 DAC，只测了墙。名字说得比做的多就是 P14-T 那个病 |
| 给 `vs_wall` 建直方图 / 抽稀 / 趋势 | 它是两个已有量的差，形状已被 `lateness` 与 `lead` 决定；三份代价换一个可算的数 |
| 改 `clock_lead_ns` 的结构 | 110 份判据在读它 |
| 把"相对墙"当成"修复了音画同步" | 这一卡只让报告**说真话**，一个采样都没重新对齐 |
| 用注入器证明"历史那 427 ms 是阶跃" | 注入只能证明"阶跃会被这样报"，不能证明当时就是阶跃。历史成因停在"必非速率 + 量级吻合"，要坐实得抓到原始 `getTimestamp` 序列（另立卡） |

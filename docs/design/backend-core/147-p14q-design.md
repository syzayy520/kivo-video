# P14-Q · 节拍侧的账要能加：`frames_decided = presented + dropped + 未交付`

> 一张卡只答一个问题。本卡答：**一帧被决策之后，它去哪了？**
>
> 起因不是设计推演，是一份真机证据写着 `frames_decided = 1800 / presented = 84 / dropped = 0`，
> 而 `drop_ratio = 0.0`。1716 帧凭空消失，全场没有任何判据看这个等式。
> 这份证据被我上一张卡 `rm` 掉了（教训记在 `146-p14p-status.md` §四），
> 所以本卡第一步是**重新造一份**，而不是引用一个查不到的数。

---

## 一、问题：两条链各记各的，没人对账

一帧从解码器出来到上屏，经过两处记账：

| 记账处 | 谁写的 | 什么时刻 |
|---|---|---|
| `pace.frames_decided` | `FramePacer::arrive()` 开头 `++frames_decided_` | **决策那一刻** |
| `pace.dropped` | `arrive()` 内 kDrop / kCatchUp 分支 `stats_.note_dropped()` | **决策那一刻** |
| `pace.presented` | `decode_receive_worker.cxx` 真正上屏后 `pacer->note_presented()` | **交付之后** |
| `video.frames_discarded_no_surface` | 换面空窗期，`render` 被强制置 false | **交付之后** |

四种动作 `kWait / kPresent / kDrop / kCatchUp` 覆盖了决策，但**只覆盖到"决定"**。
`kWait` / `kPresent` 之后还有一段路：release → Surface → 真正显示。
那段路上有两种结局没被 `presented` / `dropped` 记：

1. **没有面可上**（换面空窗）—— 已由 `video.frames_discarded_no_surface` 单独记，
   且 `decode_receive_worker.cxx:222` 有注释明确说"不能再记一次 dropped"。
2. **会话在交付之前结束**（屏幕熄灭、切后台、异常退出）—— **没有任何计数器**。

第 1 种是有意设计的，第 2 种是漏的。两种在报告里长得一样：都是
`presented + dropped < frames_decided`，而**没有任何人算过这个差**。

### 真实反例（P14-P 取证事故，数字来自会话记录）

| 字段 | 值 |
|---|---|
| `frames_decided` | 1800 |
| `presented` | **84** |
| `dropped` | 0 |
| `drop_ratio` | **0.0** |

屏幕在第 2.8 秒熄灭。解码器走完全部 1800 帧（每帧都 `arrive()`），
但只有 84 帧真的交付。差额 1716 帧在报告里**不占任何一格**。

`verify_p12c_sync.py` 校验的是**解码侧**的账
（`frames_received == frames_rendered + frames_dropped_late + frames_discarded_no_surface + 1`），
那本账是闭合的。**节拍侧这本，从来没人翻开过。**

---

## 二、先量一遍：这个等式在既有证据上成立吗

判据之前先做事实测量（213 份 `kivo.p12c.sync*`，跳过 `superseded/` 与 `emulator/`）：

| 事实 | 结果 |
|---|---|
| 三格（`frames_decided` / `presented` / `dropped`）齐全 | **213 / 213** |
| `frames_decided == presented + dropped + frames_discarded_no_surface` | **213 / 213** |
| 差额非 0 | **4 份** |

那 4 份全在 `p12d`（换面轮次），差额与 `frames_discarded_no_surface` 逐个相等：

| 证据 | decided | presented | dropped | 差额 | `discarded_no_surface` |
|---|---|---|---|---|---|
| `p12d1-rebuild-r1-sync` | 180 | 165 | 0 | 15 | 15 |
| `p12d1-rebuild-r2-sync` | 180 | 149 | 0 | 31 | 31 |
| `p12d1-rebuild-r3-sync` | 180 | 120 | 0 | 60 | 60 |
| `p12d4-d1reg-r1-sync` | 180 | 163 | 0 | 17 | 17 |

**结论：实现一直是对的，只是没人验过。** 这与 P14-M 的处境一模一样
（"138 份证据从未被任何判据看过，总收口却报四层通过"）——
区别是这次的等式**当场全绿**，不是当场炸红。

这个差别很重要，它决定了本卡怎么做：

- P14-M 是"判据错了"→ 修判据。
- 本卡是"判据缺席"→ **补判据，并且必须自证它能红**。
  一个从来没红过的判据，和没有判据是一样的。

---

## 三、自然家族树

### 3.1 这一格住哪

```
core/pace/
├── pace_stats.hpp          ← PaceSummary：加 undelivered_frames 一格
├── pace_stats.cxx          ← 不动（它看不到 frames_decided，见 P14-L 同款理由）
└── frame_pacer.cxx         ← summary()：唯一同时看得到两个数的地方
```

`PaceStats::summary()` 里 `frames_decided` 恒为 0 —— 它由 `FramePacer` 私有持有。
这与 P14-L 把 `catch_up_measured` 放进 `FramePacer::summary()` 是同一条理由：
**在"确实知道"的那一层算，不在"猜"的那一层算。**

### 3.2 为什么不带 `measured`

P14-L / P14-N / P14-P 都给新字段配了 `measured` / `reason`。
**本卡不配**，因为这一格不是测量，是**纯算术恒等式**：
`undelivered = decided − presented − dropped`，三个输入都是计数器，没有"量具够不够"的问题。

它的诚实性问题不在"测没测到"，在"差额有没有原因"。那由判据 S2 用
`frames_discarded_no_surface` 回答，不需要在数据里再写一份布尔量。

**给一个纯派生量配 `measured`，等于给 0 配一个"我很确定"** —— 那是 P14-L 反的那类东西。

### 3.3 报告层

```
pace: {
  frames_decided: 1800,
  presented: 1800,
  dropped: 0,
  undelivered_frames: 0,      ← 新增，紧跟 dropped
  ...
}
video: {
  frames_discarded_no_surface: 0,   ← 已存在，S2 跨块读它
}
```

放 `dropped` 之后，是因为这三个数必须连着读：
`1800 = 1800 + 0 + 0`。隔开就成了一堆无主字段（与 P13-N 把 `p50_range` 紧跟 `p50` 同一条理由）。

---

## 四、判据（`verify_p13d_drift.py`，S 组）

**与 Q/R 组最大的不同：S 组对全部版本生效，不是 `_schemas_from("/28")`。**

理由：这个等式从 `sync/1` 就成立（213/213 已证），本卡新增的 `undelivered_frames`
只是把差额印出来，判据本身可以现算，不需要等新字段。
按版本闸门会把 213 份的覆盖白白丢掉 —— 而 P14-M 的全部教训就是"别丢覆盖"。

| 编号 | 判据 | 能抓什么 |
|---|---|---|
| **S0** | 三格在场（`frames_decided` / `presented` / `dropped`） | 字段被删/改名 |
| **S1** | `decided == presented + dropped + undelivered`（若 `undelivered` 在场） | 手改了 `undelivered` 而没改三个源数 |
| **S2** | `decided − presented − dropped == frames_discarded_no_surface` | **核心**：差额没有原因 = 帧凭空消失 |
| **S3** | `0 <= undelivered <= decided` 且 `presented + dropped <= decided` | 重复记账（超过 100%） |
| **S4** | 报告里的 `undelivered_frames` 与现算值逐位相等（`/28` 起） | 报告层算错/印错 |

S2 是这张卡存在的理由。它在 P14-P 那份 `84 / 1800` 上会红：
差额 1716，而 `frames_discarded_no_surface = 0`。

S3 单独立一条而不是并进 S2：S2 判"有没有原因"，S3 判"账有没有超"。
`presented + dropped > decided` 时 S2 的差是负数，`!= dns`（dns >= 0）也会红 ——
但红在 S2 上会让人以为是"少记了原因"，实际是"多记了帧"。**判据的报错要说对病因。**

### 4.1 为什么 S2 不用"差额必须为 0"

4 份 `p12d` 证据差额非 0 且**合法**（换面空窗期本来就不该记 presented）。
若写成"必须为 0"，那 4 份会红，而它们是对的 —— 判据一上来就误伤，
下一个人会把它调松，然后就再也没人看它了（这正是 P14-M 修的那四处陈旧判据的来历）。

正确的写法是"差额必须有原因"，原因就是 `frames_discarded_no_surface`。

---

## 五、实施顺序

1. **核心层**：`PaceSummary` 加 `undelivered_frames`；`FramePacer::summary()` 里算。
2. **报告层**：`av_sync_report.cxx` 在 `dropped` 后印一格；`kSchema` `/27` → `/28`。
3. **判据层**：S0–S4 + `SCHEMA_LATEST` 28；`verify_p12_all.py` 两张表登记 `/28`。
4. **单测**：`FramePacer` 的 summary 在四种结局下的 `undelivered` 值。
5. **真机**：正常一轮（`undelivered == 0`）+ 受控切后台一轮（`undelivered > 0` 且能归因）。
6. **变异自测**：S1–S4 每条至少一个能红的样本；S2 用 84/1800 那个形状。

---

## 六、预言（写完不改，错了也不改回去迁就结果）

| 预言 | 值 | 依据 |
|---|---|---|
| 213 份历史证据 S2 全绿 | 213 / 213 | 已实测 |
| 差额非 0 的份数 | 4 | 已实测，全在 `p12d` |
| 正常真机轮 `undelivered_frames` | 0 | 完整播放，0 丢帧 |
| 切后台那轮 `undelivered_frames` | **> 0** | `presented` 停在切走那一刻，而解码继续 |
| 切后台那轮 S2 | 取决于 `discarded_no_surface` | **不确定** —— 见下 |

最后一条我**故意不预言**：切后台时 Surface 是否 detached 决定 `dns` 是否为 0。
- 若 `dns = undelivered` → S2 绿，说明"切后台"走的是换面路径，账早有解释。
- 若 `dns = 0` → S2 红，说明存在**第三种出口**，P14-P 那个 1716 就是它。

**两种结果都有价值**，但只有第二种能让这张卡兑现"有真实反例"的承诺。
设计时不预设哪一种，测出来是什么就写什么。

---

## 七、明确不做

1. **不修那个缺陷。** 若确认存在"会话结束丢账"，本卡只负责**让它可见**。
   修它要动会话收尾路径（谁在 stop 时补记），那是另一张卡。
2. **不碰 `presented != frames_rendered` 那 10 份。**
   它们是 seek 轮次：`presented` 累计、`frames_rendered` 被 `begin_run()` 重置。
   语义差异已知，不在本卡范围。
3. **不给 `undelivered` 配 `measured`**（§3.2）。
4. **不动 `closure_gap_ns`。** 那是时间域的残差（ns），本卡是帧数域的残差（帧）。
   两个都叫"残差"，但回答的问题不同：那个说"这一段的钟账差多少 ns"，
   这个说"这一段有几帧没落地"。混成一个会同时失去两个信息。
5. **不动解码侧的账。** `verify_p12c_sync.py` 那本已经闭合且有判据。

# 85 · P13-O 验收：两个半程斜率必须自己说出「差多少算差」

- 版本：V1.0
- 日期：2026-10-30
- 设计：`84-p13o-design.md`
- 判据：O0–O7（见设计 §六）
- 结论：**达成**。`slope_halves_agree` 降为历史字段，新增 16 格数值 + 三态；
  `inconsistent ⇒ drift_verdict = unknown` 已接线，并由端到端单测覆盖。

---

## 一、改了什么（一句话版）

以前读者只能看到一个布尔「两个半程同不同号」；现在能看到：

```
两半程各自的斜率 + HAC 标准误 + 95% 带
gap（差多少） + pooled（这个差的抽样误差） + z（差几个 σ）
cumulative_gap_ns（两个累积量差多少纳秒）
resolution_ppm（这套证据能分辨的最小差） vs budget_equivalent_ppm（要分辨"会积满额度"所需的差）
relation：consistent / inconsistent / unknown
```

**关键是把"差多少"和"差多少算差"一起出版。** 只给 `gap` 会让读者拿
0.57 ppm 的差去和一把不存在的尺子比。

---

## 二、真机四轮（`docs/evidence/p13o/`，schema `kivo.p12c.sync/13`）

片源 `p13h_360p30_130s_av.mp4`（除 short 用 35 s 片），全部 `cpu_paced`（否则 `av_drift` 不可测）。

| 轮 | 前半 ppm | 后半 ppm | 旧布尔 `agree` | 新判据 | z | gap / pooled | resolution / budget |
|---|---|---|---|---|---|---|---|
| plain | +1.727 | +0.574 | **true** | `consistent` | 0.873 | 1.154 / 1.321 | 2.59 / 439.6 |
| fwd（seek→90 s） | −6.726 | +1.082 | **false** | `consistent` | 1.580 | 7.808 / 4.942 | 9.69 / 1125.9 |
| short（35 s 片） | −4.508 | +0.146 | **false** | `consistent` | 1.599 | 4.654 / 2.911 | 5.70 / 1286.9 |
| back（seek→5 s） | +0.245 | +0.674 | **true** | `consistent` | 0.621 | 0.429 / 0.691 | 1.35 / 439.6 |

四轮 `resolution_sufficient` 全为 true（resolution 比额度等效尺度小两个数量级），
`drift_verdict` 无一轮被降级（因为没有一轮 `inconsistent`）。

### 2.1 最有价值的一行：旧布尔两次说「不一致」，新判据说「在噪声内」

`fwd` 与 `short` 两轮，两个半程**反号**（−6.7 / +1.1 与 −4.5 / +0.15），
旧布尔 `slope_halves_agree = false`，读起来是"两段不是一回事"。

但差值只有 **1.58σ / 1.60σ** —— 落在噪声里。真实情形是**一次掉头**（V 形漂移的两条腿），
不是"两段不同的漂移"。设计 §1.4 里写的那行表格（"−0.5 / +0.5：反号，但可能是中间掉头"）
在这四轮里被真机撞上两次。

**这就是"同号是最弱的相似性"的实证**：它连"这两个数差多少"都不问，
于是既会把 21 倍的差异说成一致（P13-N 的 `p13n-fwd-r1`），
也会把一次掉头说成不一致（本轮的 fwd / short）。

---

## 三、`inconsistent` 分支怎么验的（设备没撞上）

四轮设备证据**恰好全部 `consistent`**，所以 `inconsistent ⇒ drift_verdict = unknown`
这条分支在真机上没被触发。它由两处覆盖：

1. **端到端单测 O12**：造一条 60 s 序列，前 30 s 走 0.2 ppm、后 30 s 走 4 ppm，
   断言 `relation == kInconsistent` **且** `drift_verdict == kUnknown` **且**
   `slope_ppm > 0`（证明降级不是因为"没样本"，是那条直线描述错了对象）。
   对照组 O11（单斜率 0.5 ppm）断言 `consistent` 且 verdict 不被降级。
2. **校验器 O7a**：`inconsistent ⇒ verdict == unknown`，对着任何一份 `/13` 证据都会判。

> 不把这一条留给"以后自然会撞上"：一份两半程不是一条线的证据，
> 如果不降级，它照样会报 `within_budget` —— 而那个数是一条错直线乘出来的。

---

## 四、变异自检（两处，都能打红）

| 变异 | 结果 |
|---|---|
| M1 判据反号（`gap > 1.96·pooled` 写成 `<`） | **5 项断言红** |
| M2 去掉 `isfinite` / 非负检查（NaN 会滑进 `consistent`） | **6 项断言红**（O4 三条全红） |

M2 是这张卡最危险的一条路：`NaN > x` 恒为 false，漏查就会把"算不出来"
判成"一致"。O4 的三条断言（NaN / +inf / 负 SE）专门守它。

---

## 五、单测与收口

- `kivo_p13o_halves_tests`：**12 例 / 57 断言**（O1–O10 纯函数，O11–O12 端到端）
- `ctest -R kivo_p13`：13/13
- 总收口 `python tools/verify_p12_all.py`：**390 份 / 四层全通过 / 8 个子校验器 exit 0**
  （新增 8 份证据：p13o 四轮的 decode + sync）

---

## 六、诚实清单

1. **`slope_halves_agree` 没有删。** 它保留原语义（同号），现在紧挨着新三态，
   位置是**历史字段**。删掉会让 `/9`–`/12` 的证据无法对读，改语义会让同一个
   字段名在两版证据里指两件事。**读证据时该看的是 `slope_halves_relation`。**
2. **`inconsistent` 没在真机上出现过。** 四轮全是 `consistent`。该分支由 O12 单测 +
   O7a 校验器覆盖，不是真机证据。
3. **`fwd` / `short` 的 `resolution` 比 `plain` 差 3–4 倍**（9.7 / 5.7 vs 2.6 ppm）：
   半程只有 20 s / 17 s，样本少，SE 大。这恰恰是 `resolution_sufficient` 存在的理由 ——
   在那两轮里说"一致"，其信息量本来就比 `plain` 弱。
4. **`trend` 窗口可以跨 seek 段。** `fwd` 与 `back` 的 `segments = 2`，
   但半程比较仍把整个窗口当成一条线。**这是一个已知的口径问题，本卡没修**：
   该不该在 `segments > 1` 时直接给 `unknown`，需要单独论证（否则一次正常 seek
   就会让漂移判据永远不可用）。记在这里，别当作已解决。
5. **只有一台设备、两种片源**（360p30 130 s / 35 s）。4K / 高码率未测。
6. **`z` 用 1.96 是渐近正态的临界值**，而 HAC 只在大样本下才近似正态。
   半程各 20 s（约 400 个抽稀点）时这个近似勉强站得住，更短的片段不该用同一把尺子。
7. **`budget_equivalent_ppm` 用的是 `kAudibleThresholdNs = 45 ms`**，与 P13-L 同一把尺子；
   改额度会同时改变这张卡的"够不够"结论。

---

## 七、产物

- 设计：`84-p13o-design.md`
- 新增：`core/pace/pace_halves_agreement.{hpp,cxx}`（纯判定）、
  `tests/video/pace/p13o_halves_tests/halves_tests.cpp`
- 改：`pace_hac`（区间版 `estimate_hac_slope_error_range`）、
  `pace_drift_trend`（两半程 HAC + 比较 + verdict 降级）、
  `av_sync_report`（`sync/13`，16 格 + 三态）、
  `tools/verify_p13d_drift.py`（O0–O7）、`tools/verify_p12_all.py`（登记 `/13`）
- 证据：`docs/evidence/p13o/p13o-{plain,fwd,short,back}-r1{,-sync}.json`（8 份）

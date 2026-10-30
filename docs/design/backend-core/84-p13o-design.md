# 84 · P13-O 设计：两个半程斜率必须自己说出「差多少算差」

> 卡片：P13-O
> 前置：P13-K（`76-p13k-design.md`）引入半程斜率用作「持续漂移 vs 中途跳一下」的诊断
> 结论：给两个半程各自的 HAC 标准误与置信带，把 `slope_halves_agree`（只比符号）
> 降级为历史字段，新增四格数值 + 三态判据；`inconsistent` 时 `drift_verdict` 降为 `unknown`
> schema：`kivo.p12c.sync/12` → `/13`

---

## 一、问题

### 1.1 一个布尔在真机上说过三次谎

`slope_halves_agree` 的定义只有一句：两个半程斜率**同号**。

拿项目里已有的 13 份真机 sync 证据对一遍：

| 证据 | 前半 ppm | 后半 ppm | 差 | 量级倍率 | `agree` | 全序列 ppm |
|---|---|---|---|---|---|---|
| `p13k-130s-r1` | +1.1767 | +1.0081 | 0.169 | 1.2 | true | +0.2822 |
| `p13k-130s-seek-r1` | +1.4417 | +0.1870 | 1.255 | **7.7** | true | +0.0952 |
| `p13k-35s-r1` | −3.3389 | +1.0514 | 4.390 | — | false | +0.3069 |
| `p13l-130s-r1` | +0.3972 | +0.2203 | 0.177 | 1.8 | true | −0.2379 |
| `p13l-35s-r1` | −2.9145 | −0.2173 | 2.697 | **13.4** | true | −0.9978 |
| `p13l-seek-back-r1` | −0.6379 | −0.5463 | 0.092 | 1.2 | true | −0.3433 |
| `p13m-back-r1` | +0.7076 | −0.2522 | 0.960 | — | false | +0.1300 |
| `p13m-freeze-r1` | −0.3929 | +0.5044 | 0.897 | — | false | −0.0313 |
| `p13m-fwd-r1` | −0.3311 | −0.1762 | 0.155 | 1.9 | true | −0.0810 |
| `p13m-plain-r1` | −0.2391 | −0.1490 | 0.090 | 1.6 | true | −0.0738 |
| `p13n-fwd-r1` | +0.5975 | +0.0276 | 0.570 | **21.7** | true | **−0.0015** |
| `p13n-plain-r1` | −0.4273 | +0.6326 | 1.060 | — | false | +0.1486 |
| `p13n-short-r1` | +0.9717 | −2.0524 | 3.024 | — | false | +1.2505 |

8 份 `agree = true` 里，**3 份的两个半程量级差 5 倍以上**（7.7 / 13.4 / 21.7 倍）。

字段注释其实已经承认过它不可靠：

> ⚠️ 它只说方向，不说大小，也没有阈值：斜率本身接近 0 时符号由噪声决定，
> 这时 `true` 与 `false` 都是掷硬币。

但**承认了还要出版**，就等于把一枚硬币印成结论。读者看到 `true`，
读到的必然是「整段朝一个方向走，所以全序列斜率可以外推」。

### 1.2 最刺眼的一份：两半程为正，全序列为负

`p13n-fwd-r1`（130 s 片源，向前 seek）：

| 字段 | 值 |
|---|---|
| `slope_first_half_ppm` | **+0.5975** |
| `slope_second_half_ppm` | **+0.0276** |
| `slope_halves_agree` | **true** |
| `slope_ppm`（全序列） | **−0.0015** |

前半程把斜率积在自己的 65 s 上：`0.5975 ppm × 65 s = 38.8 µs`。
后半程只有 `0.0276 × 65 = 1.8 µs`。
全序列那条线却报 `−0.0015 × 130 s = −0.2 µs`。

三个数对同一段播放给出三种答案，而 `agree = true` 在说「它们是一致的」。
真实情形是：**前 65 秒在漂，后 65 秒停了**，中间那一跳把整条回归线的符号翻了过来。

这正是 `slope_halves_agree` 一开始想区分的那件事 —— 它没做到，因为它只比符号。

### 1.3 半程斜率没有误差棒：P13-J 修过一半的病

P13-J 给全序列斜率配了标准误，理由是：六位小数的裸数会被读成「精确测到」。

```
slope_ppm = -0.022183      ← 六位小数
r2        = 5.5e-05        ← 什么都没解释
```

同一个病在半程这一支上**原封不动**：`slope_first_half_ppm` 与
`slope_second_half_ppm` 至今是两格裸数，旁边既没有标准误，也没有「这两个数差多少才算差」。

所以读者拿到的信息是：

- `+0.5975` 与 `+0.0276` 都是六位小数的精确数；
- 判定 `true` 说它们同号；
- **没有任何一个字段告诉他这两个数的差异是不是噪声。**

### 1.4 「同号」是最弱的相似性

若把两个半程看成对同一个量的两次独立测量，要问的问题只有一个是明确的：
**两者的差异相对于各自的抽样误差有多大。**

同号连这个问题的边都没碰到：

| 情形 | 同号？ | 是同一个东西吗 |
|---|---|---|
| +0.030 / +0.028 | 是 | 是 |
| +3000 / +0.03 | 是 | **不是**（前者是时钟速率差，后者是零漂） |
| +0.001 / +0.002 | 是 | **不知道**（两个都在噪声里） |
| −0.5 / +0.5 | 否 | **是中间掉头了**（不是"两段不同"） |

最后一行说明连 `false` 也不可信：反号被它当成「不一致」，而反号可能只是
一个 V 形漂移的两条腿。

---

## 二、判据设计

### 2.1 四格数值

两个半程各拟合一次（各 `n/2` 点），各拿一个 HAC 标准误与置信带：

```
first  = slope_first_half_ppm          second = slope_second_half_ppm
se1    = first_half_std_error_ppm      se2    = second_half_std_error_ppm
gap    = |first − second|
pooled = sqrt(se1² + se2²)             ← 差值本身的抽样误差（两侧独立）
z      = gap / pooled
```

`gap` 是**差值**，`pooled` 是**这个差值的误差**。两者相除才有意义；
单独出版 `gap` 而不出版 `pooled`，读者会拿一个 0.57 ppm 的差去和一个 0.18 ppm 的尺子比
—— 而那个比较根本不成立。

### 2.2 三态

```cpp
enum class HalvesRelation : std::int8_t {
    kUnknown,       // 至少一侧的 SE 估不出来 —— 不是"一致"
    kConsistent,    // 差在噪声之内
    kInconsistent,  // 差超出噪声 —— 全序列斜率描述错了对象
};
```

判据（按顺序）：

1. 任一侧 `estimated == false`，或 SE 非有限 / 为负 → `kUnknown`
2. `gap > 1.96 × pooled` → `kInconsistent`
3. 否则 → `kConsistent`

第 2 条就是「两个 95% 置信带是否重叠」的 z 检验形式。
用 z 而不用「区间端点相交」，是因为端点相交在两侧 SE 相差很大时会被
「宽的那一侧把窄的一侧整个包住」骗过去 —— 而那种情形恰恰是最该报不一致的。

**三态不是布尔**：`kUnknown` 与 `kConsistent` 印成同一个 `false` 的话，
"算不出来"就会伪装成"算出来是一致的"。这个项目在 P13-L 的 `drift_verdict` 上
已经为同一个错付过一次代价。

### 2.3 尺子够不够：`resolution_ppm` 与 `budget_equivalent_ppm`

`kConsistent` 只是说「差落在噪声里」。它**强不强**，取决于这把尺子有多细。

```
resolution_ppm        = 1.96 × sqrt(se1² + se2²)   ← 这套证据能分辨的最小速率差
budget_equivalent_ppm = 45_000_000 / span_ns × 1e6 ← 要分辨出"会积满额度"所需的速率差
resolution_sufficient = resolution_ppm <= budget_equivalent_ppm
```

为什么要有第二个数：`kConsistent` 在任何 SE 下都可能成立 —— 只要两侧的噪声
都足够大，任何差异都会被吞掉。一份半程各 15 个点的证据，`resolution_ppm`
可以到 40 ppm；它在 130 s 上是 `346 ppm` 的等效尺子下勉强够用，在 30 s 短片
（`1500 ppm`）上当然也够用，但**在它自己的分辨力之下，"一致"这个词没有信息量**。

所以这个布尔不进三态（它不改变"差在不在噪声内"这个事实），
而是作为一个**保证条件**出版：校验器判
`consistent ⇒ resolution_sufficient`，让「空口说一致」在结构上不可能。

### 2.4 为什么不把门槛设成 45 ms 直接切

试过这条路，结论是它会造出一个永不触发的分支：

- 半程 SE ≈ 全序列 SE 的 2 倍（`n` 减半 ⇒ `Sxx` 降到约 1/4）；
- 真机全序列 SE 在 0.086 ~ 0.925 ppm；
- 于是 `resolution_ppm` 在 **0.5 ~ 5 ppm**；
- 而 45 ms 额度在 130 s 上等效 `346 ppm`、在 35 s 上等效 `1286 ppm`。

`resolution` 比额度尺度小两个数量级，任何「够不够」的判断都恒为够。
那样的分支只会让判据看起来更完整，不会让证据更可信 ——
P13-M 的 C6 变异（把 `max` 改成累加）在 13 条用例下全部存活，
就是同一个教训：**没有能触发它的样本的分支，等于没有这个分支。**

所以这张卡把「够不够」做成一个可读的布尔 + 两个可比的数，
让校验器去守它，而不是塞进三态里假装有第四条路。

### 2.5 `drift_verdict` 的降级

P13-L 的头注释里已经写明：

> ⚠️ 它只覆盖**斜率的抽样误差**，不覆盖模型失配……那种情况由
> `slope_halves_agree` 报警，不是靠把上界放大能救的。

而 `slope_halves_agree` 报不了警（§1.1）。于是现在的 `within_budget`
在一份两半程反号的证据上照样成立 —— 它算的是「一条不描述这段播放的直线
乘以这段播放的跨度」。

规则：**`kInconsistent ⇒ drift_verdict = kUnknown`。**

理由与 `usable == false ⇒ kUnknown` 是同一条：乘法没有定义域时，
乘积不是"小"，是没有意义。`kConsistent` 与 `kUnknown`（半程无法比较）
都不改动 `drift_verdict` —— 前者是支持，后者是没意见，
只有 `kInconsistent` 是**反证**。

---

## 三、自然家族树

```
backend/include_private/video/video_plane/core/pace/
├── pace_hac.hpp                 改：+ estimate_hac_slope_error_range(begin, end, slope)
├── pace_halves_agreement.hpp   新：HalvesRelation / HalvesAgreement / compare_halves
├── pace_drift_trend.hpp        改：+ 12 格新字段；slope_halves_agree 标注 legacy
backend/src/video/video_plane/core/pace/
├── pace_hac.cxx                改：区间版；原入口转发到 [0, n)
├── pace_halves_agreement.cxx   新：纯函数，无序列、无时钟、无分配
├── pace_drift_trend.cxx        改：半程拟合保存 slope_per_ns → 两把区间 HAC → 接线
backend/src/video/video_plane/backends/avsync/report/
└── av_sync_report.cxx          改：sync/13，旧布尔后紧跟新四格 + 三态 + 保证条件
backend/tests/video/pace/p13o_halves_tests/
└── halves_tests.cpp            新
tools/verify_p13d_drift.py      改：SCHEMA_ORDER → /13，check_halves_relation(O0–O7)
tools/verify_p12_all.py         改：两处 schema 表登记 /13
```

**为什么 `pace_halves_agreement` 独立成单元**：它不读序列、不读时钟、不分配，
输入是六个标量。放进 `pace_drift_trend` 会让那个文件同时承担「拟合」
与「判两个拟合是否相容」两件事 —— 而后者是这张卡唯一的新知识，
它需要能被单独测试、单独变异。

**为什么区间 HAC 放回 `pace_hac` 而不是新单元**：职责仍是「一条直线的 HAC 斜率
标准误」，只是把定义域从整条序列收窄到一段。另立单元会让同一套 Bartlett 核
出现两份实现，而两份核迟早会在 `L` 的钳制上分家。

---

## 四、字段清单（`sync/13` 的 `trend` 块）

紧跟 `slope_halves_agree` 之后，顺序即阅读顺序：

| 字段 | 类型 | 含义 |
|---|---|---|
| `slope_first_half_std_error_ppm` | float | 前半程 HAC 标准误 |
| `slope_second_half_std_error_ppm` | float | 后半程 HAC 标准误 |
| `slope_first_half_ci_low_ppm` / `_high_ppm` | float | 前半程 95% 带 |
| `slope_second_half_ci_low_ppm` / `_high_ppm` | float | 后半程 95% 带 |
| `slope_halves_gap_ppm` | float | `abs(first − second)` |
| `slope_halves_pooled_error_ppm` | float | `sqrt(se1² + se2²)` |
| `slope_halves_z` | float | `gap / pooled` |
| `slope_halves_cumulative_gap_ns` | int | 两半程各自积在自己跨度上的累积量之差 |
| `slope_halves_resolution_ppm` | float | `1.96 × pooled`，这套证据能分辨的最小差 |
| `slope_halves_budget_equivalent_ppm` | float | 额度除以跨度，要分辨出「会积满额度」所需的差 |
| `slope_halves_resolution_sufficient` | bool | `resolution ≤ budget_equivalent` |
| `slope_halves_relation` | string | `unknown` / `consistent` / `inconsistent` |

`slope_halves_agree` **保留原语义**（同号），不删不改：
删掉会让旧证据无法对读，改语义会让同一字段名在两版证据里指两件事。
它现在的位置是历史字段，紧挨着的新三态才是该读的那一格 ——
这条必须出现在状态卡的诚实清单第一条。

`slope_halves_cumulative_gap_ns` 的算法：

```
前半程跨度 = at(half−1).media_ns − at(0).media_ns
后半程跨度 = at(n−1).media_ns − at(half).media_ns
gap_ns     = |first × 前半程跨度 − second × 后半程跨度| / 1e6
```

用**各自的真实跨度**而不是「半程时长」：两半程的长度由 `n/2` 的整数除法决定，
末尾那一个点的归属会让两者差一格；用真实跨度就不必假设它们等长。

---

## 五、不做什么

1. **不改全序列 `slope_ppm`。** 它仍是那条最小二乘线 —— 错的是读者对它的
   外推，不是这个数本身。要让读者不外推，办法是把「它描述错了对象」这件事
   出版，而不是换一条回归。
2. **不把半程改成三段或滑动窗。** `kInconsistent` 只回答「是不是一条线」，
   不回答「在哪儿断的」。定位断点是另一张卡（需要 ts 检验之类的东西），
   本轮先把「断了没有」这一格做对。
3. **不改 `slope_halves_agree` 的语义。** 见 §四。
4. **不给 `kInconclusive` 造第四条路。** 见 §2.4。
5. **不动 `kAudibleThresholdNs = 45 ms`。** 它是 P13-L 的额度，
   这张卡只拿它换算一个等效尺度，不改它。

---

## 六、验收

| 编号 | 判据 |
|---|---|
| O0 | 12 个新字段齐全 |
| O1 | 两侧 SE 有限且非负 |
| O2 | 两侧 CI = 各自 `slope ± 1.96·SE` |
| O3 | `pooled = sqrt(se1² + se2²)` 可复算 |
| O4 | `z = gap / pooled` 可复算 |
| O5 | 三态与 `z` 自洽：`z > 1.96 ⇔ inconsistent` |
| O6 | `cumulative_gap_ns` 与两半程斜率、跨度自洽 |
| O7 | `inconsistent ⇒ drift_verdict == unknown`；`resolution_sufficient` 与两个 ppm 自洽 |

真机验收（`docs/evidence/p13o/`）：

- 平跑 130 s：预期 `consistent`，`drift_verdict` 不变；
- 向前 seek：预期 `inconsistent`（复现 `p13n-fwd-r1` 的形状），`verdict` 降 `unknown`；
- 短片 35 s：预期 SE 大、`resolution` 大，用来验保证条件不是空话。

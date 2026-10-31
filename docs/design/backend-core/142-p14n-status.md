# 142 · P14-N 验收：`sync` 侧的可测性终于和 `decode` 侧说同一句话

- 设计：`141-p14n-design.md`
- 证据：`docs/evidence/p14n/`（1080p30 / 60 s / 同一片源 / 只换排程档）
- 改动：`sync/24` → `/25` —— `pace.over_budget_frames` 与 `pace.lateness_ns`
  各带一对 `measured` / `reason`，键名与 `decode/18` 逐字一致
- 校验：`verify_p13d_drift` Q 组 **2/2 份通过（含 `not_measured` 那份）**；
  全量 `verify_p12_all` **644 份 / 四层全通过 / 20 个子校验器 exit 0**
- 门禁：`ctest -C Debug -R "kivo_p1[234]"` **25/25**
- 真机：K40（`b6e950ee`）两档各一轮

---

## 一、这一卡要证的一件事

> 同一轮播放，两份报告对"lateness 量到了没有"必须给出同一个答案。

P14-L 把 `decode` 侧改好了，`sync` 侧没改。改之前，同一台设备、同一片源、
只换排程档，`sync/24` 印出来是这样：

| 项 | `cpu_paced` | `surface_at_time` | 读者会怎么读右列 |
|---|---|---|---|
| `lateness_ns.samples` | **1800** | **0** | （没有这一格可读） |
| `lateness_ns.p50` | **125000** | **0** | 「零延迟」 |
| `lateness_ns.p99` | **125000** | **0** | 「最差也零延迟」 |
| `lateness_ns.max` | **723854** | **0** | 「最大迟到 0 ns」—— 最完美的最大值 |
| `over_budget_frames` | 0 | 0 | 「一帧都没超预算」 |

右列那四个 0 一个都不是测出来的：`surface_at_time` 把上屏时刻交给合成器，
`note_presented` 从不被调用，`lateness_` 直方图一个样本都没进。
`over_budget_frames` 由 `count_abs_beyond()` 从那个空直方图数出来，因此必然是 0。

**`over_budget_frames` 是 G6 的判据。** 改之前，默认排程档上的 G6 读着一个
永远为 0 的数，通过着一次从未发生的检查。

改之后（`/25`，同一批真机证据）：

| 项 | `cpu_paced` | `surface_at_time` |
|---|---|---|
| `lateness_ns.samples` | **1800** | **0** |
| `lateness_ns.p50` / `p99` / `max` | 125000 / 125000 / 723854 | 0 / 0 / 0 |
| **`lateness_ns.measured`** | **true** | **false** |
| `lateness_ns.reason` | `""` | *"no present-timestamp samples: in surface_at_time the compositor decides when a frame is shown, so only the submit lead (schedule_lead_ns) is observable"* |
| `over_budget_frames` | 0 | 0 |
| **`over_budget_frames_measured`** | **true** | **false** |
| `over_budget_frames_reason` | `""` | 同上（同一条派生链，同一句话） |

**同一个 0，左边是「一帧都没超」，右边是「根本没量」。**

---

## 二、为什么 P14-L 当时做不了，P14-M 之后才能做

P14-L 的诚实清单第一条就写着「`sync` 报告同一个缺陷没修」，当时的判断是
"成本低，下一张卡做"。**P14-M 改变了这件事的性质**：

在 P14-M 之前，`sync` 证据**根本没被 drift 判据看过** —— `SCHEMA_ORDER` 停在 `/13`
而报告已到 `/24`，总收口 `glob` 只扫 `p13*`。204 份里有 138 份从未被看过，
而总收口一直报「四层通过」。

在那种情况下往 `sync` 里加字段，等于让新字段带着一整块没人验的区域通过总收口。
P14-M 把覆盖从 54 份补到 177 份之后，这一格才有意义 —— 这也是本卡排在 P14-M 后面的原因。

---

## 三、判据 Q 组（`verify_p13d_drift.py`，`/25` 起）

六条，全部从 JSON 独立复算，不看 C++ 实现：

| 编号 | 判据 | 抓什么 |
|---|---|---|
| Q0 | `/25` 起两对字段必须齐全 | 漏写 |
| Q1 | `measured=true` ⇒ `reason == ""` | 自相矛盾（量到了却带理由） |
| Q2 | `measured=false` ⇒ `reason` 非空且不是那句证词 | "没量到但不说为什么" |
| Q3 | `measured=false` ⇒ 对应值必须全 0 | 没量到却报了非零（比沉默的零更糟） |
| Q4 | `lateness.measured == (samples > 0)` | 可测性与样本数分家 |
| Q5 | `over_budget_measured == lateness.measured` | 同一条派生链被各判一次 |

Q3 是单向的：量到了也可以是 0（真的没有超预算帧），所以不能写成双向。

### 3.1 判据放在哪里，决定了它在哪些证据上存在

**这是本卡第一个必须记下来的错。**

Q 组第一次被写在 `check_file()` 的末尾。跑变异时，`cpu_paced` 那份的 Q0/Q1 都红了，
但 `surface_at_time` 那份的 Q2/Q3/Q4/Q5 **全部存活**。

原因：`surface_at_time` 档的 `av_drift.mode = not_measured`，而那条路在 F 段之前就
`return` 了。于是 Q 组**恰好在最需要它的那批证据上根本没跑**，而输出仍是「全部通过」。

```
$ python tools/verify_p13d_drift.py <一份 surface_at_time 的 /25 证据>
✓ schema 受支持
✓ E1 … ✓ not_measured ⇒ 也带 reason
覆盖：1 份（判据真的跑了）
结论：全部通过。          ← 六条 Q 判据一条没出现
```

修法：把 Q 组移到 `check_file()` 的**任何 `return` 之前**（紧跟 `schema 受支持`）。
它读的是 `pace` 块，与 `av_drift` 没有任何关系，本来就不该跟在那条链后面。

这与 P14-M 是同一个病：**不是判据写错，是判据没被执行，而没人看得出来。**
P14-M 修的是"版本不认识 ⇒ 跳过"，本卡修的是"提前 return ⇒ 跳过"。

### 3.2 变异：12 条，全部按预期

| 变异 | 命中 | 说明 |
|---|---|---|
| 正例 `cpu_paced`（measured=true, 1800 样本） | 全绿 | |
| 正例 `surface_at_time`（measured=false, 0 样本 + 理由） | 全绿 | |
| Q0 删 `over_budget_frames_reason` | **Q0** | |
| Q1 `over_budget` 量到却带理由 | **Q1** | |
| Q1b `lateness` 量到却带理由 | **Q1** | 另一支 |
| Q2a `lateness` 空理由 | **Q2** | |
| Q2b 理由 = 那句证词 | **Q2** | |
| Q2c `over_budget` 空理由 | **Q2**（连带 Q5） | |
| Q3 `lateness.max = 12345` 而 measured=false | **Q3** | |
| Q3b `over_budget_frames = 7` 而 measured=false | **Q3** | |
| Q4 `measured=false` 但 `samples=1800` | **Q3**（先被 Q3 抓到） | |
| Q4b `measured=true` 但 `samples=0` | **Q4**（只有 Q4 能抓） | 证明 Q4 独立有效 |
| Q5 链分家（over 说量到、lateness 说没量到） | **Q5** | |
| Q5b 反向分家 | **Q5** | |

Q4 的第一版变异被 Q3 抢先抓到（`samples=1800` 而 `measured=false`，Q3 也红）。
所以补了 Q4b —— 只让 Q4 红的那个方向。**一条判据必须有一个只有它能抓的样本**，
否则它可能永远是死的而没人知道。

---

## 四、一个当时全绿、其实需要改的地方

`verify_p12_all.py` 里有**两张** sync schema 表：

| 变量 | 作用 |
|---|---|
| `SYNC_QUANTILE_BLOCK_SCHEMAS` | 决定 `av_drift` 是不是 `quantiles.<口径>` 分块结构 |
| `KNOWN_SCHEMAS` | 白名单，没登记就判红 |

本卡先只改了第二张，全量校验**是绿的** —— 因为 `/25` 的证据还不存在，
没有任何一份文件能让第一张表的遗漏显形。

反向实验（删掉第一张表里 `/25` 那一行）：

```
sync_sample_count(/25 证据) = 1200      ← 正确
sync_sample_count(/25 证据) = None      ← 删掉后（顶层 av_drift.samples 在 /3 起就不存在）
is_v3 = True    → False                 ← 整套分块结构检查被静默跳过
```

这不是假设：`sync_sample_count()` 拿到 `None` 之后，`has_audio_evidence` 会判错，
而 `check_sync()` 的 `/3` 结构检查整块不跑。

**教训**：绿色的证据不是"没问题"，是"还没有东西能暴露它"。
这也是 P14-M 那一整张卡的另一面 —— 那张卡修的是"版本不认识"，这张卡撞见的是
"版本认识，但只认得一半"。

---

## 五、改动清单

| 文件 | 改了什么 |
|---|---|
| `backend/src/.../avsync/report/av_sync_report.cxx` | `over_budget_frames` 后接 `_measured` / `_reason`；`lateness_ns` 块内加 `measured` / `reason`；`kSchema` 升 `/25` + 历史注释 |
| `tools/verify_p13d_drift.py` | `SCHEMA_LATEST` 24→25；`SCHEMA_WITH_MEASURABILITY`；`check_measurability()` Q0–Q5（放在提前 return 之前） |
| `tools/verify_p12_all.py` | 两张 schema 表各登记 `/25` |
| `tools/run_p14n_sync_measured.sh` | 新：真机两档脚本 |
| `docs/design/backend-core/141-p14n-design.md` | 新：设计卡 |
| `docs/evidence/p14n/` | 新：真机两档四份 |

**没有新增任何 C++ 文件**，`core/pace/` 一行没动 —— `MeasuredCounter` 在 P14-L 已经挂好，
`PaceStats::summary()` 与 `FramePacer::summary()` 已经填好。本卡在报告层只加了 `out <<`，
一行判断都没加。

---

## 六、诚实清单（六条，按要紧程度）

1. **`av_sync_report.cxx` 只在 Android 侧编译。**
   桌面 `ctest` 覆盖不到它 —— 这条改动**没有任何桌面单测能证明它编译得过**。
   本卡的编译验证只有一条证据：Android 构建产物的对象文件时间戳
   （`av_sync_report.cxx.o` 17:34:48 晚于源文件 17:31:57），加真机证据里那两个字段确实出现了。
   这是架构事实（两份 CMakeLists 各写各的源清单），不是本卡造成的，但每次改这个文件都要记住。

2. **`schedule_lead_ns` 仍是裸数，而且是反向的沉默零。**
   `cpu_paced` 档它 0 样本、`surface_at_time` 档 1800 样本 —— 与 `lateness_ns` 正好相反。
   不加 `measured` 是刻意的（见设计卡 §二）：那一档本来就不该用它，
   加标志会把"用错尺子"模糊成"尺子没校准"。但这个决定没有判据守着。

3. **`closure_gap_ns` / 四个 `span` / `first_lateness_ns` 在 `surface_at_time` 档仍是 0。**
   它们有接线自证（`clock_supplied_frames` / `due_step_supplied` 在该档都是 0，
   小于 `presented`），所以整块可以判为作废。但"自证字段本身是 0"时怎么读，
   本卡没有处理 —— 那需要另一种形状的判据。

4. **`pace.lateness_ns.p50` 仍是桶中心（125000）。**
   P13-N 的 `p50_range_*` 只加在 `quantiles.<口径>` 下，`pace` 块这一份没有。
   所以在 `sync` 里，`lateness_ns.p50` 依然说不出自己的精度。
   这一条与 P13-N 是同一个病，只是换了个位置。

5. **Q 组只守 `sync`，`decode` 侧仍是 P9–P11。**
   两套判据语义相同、实现两份。它们迟早会分家 —— 分家的那一天，
   两份报告又会开始对同一件事说两种话。没有机制防止这一点。

6. **`SCHEMA_LATEST` 仍要手工维护。** P14-M 让它"认不出就报红"，比静默好，
   但报红时仍要人改。本卡就是那个人工步骤：`24` → `25`。

---

## 七、下一步

**给 `p99` 也出版范围。** P13-N 只给了 `p50_range`，于是 P14-M 抓到的那条
`D2 = 5.00x`（30 vs 60 fps）永远只能判「不可判」—— 30fps 的 p99 正落在第一个桶的中心
（125 µs，桶宽 250 µs），拿桶中心当分母，商可以是 2.5x 到无穷。
把桶宽从 250 µs 改成 50 µs 后同一对立刻判成 5.00x 失败 —— 所以那不是放水，是量具不够。

其次是**把 `p50_range` 补到 `pace.lateness_ns`**（诚实清单第 4 条），
再是 **G6/G7 在默认档上的真正测量**（要改呈现路径，动合成器回调，是独立的一项）。

# P13-I 状态卡：窗口的身份 —— 一个从没被问到过的字段，它的名字就是它唯一的规格

> 设计：`72-p13i-design.md` · 证据：`docs/evidence/p13i/`（3 轮：130 s / 130 s+seek / 35 s，`sync/7`）
> 前序：`71-p13h-status.md` §六.3 的两把尺子欠账。本卡去处理它，顺手撞到旁边
> 一个**真说假话**的字段 —— 那个更严重，也不在欠账里。

---

## 一、两个缺陷

### A. `truncated` 把"换过时间轴"说成"环形覆盖过"

上一版判据是 `total_ > kCapacity`，而 `total_` 是**整场累计**、`clear()` 不清它。
seek 走 `clear()`，于是"整场 3100 > 2048"被判成覆盖发生过 —— 而环形里躺着的是
第二段的**整整 2000 个点，一个都没被覆盖**。

真机数据（r2，55 s 时 seek 到 30 s）：

| 字段 | 值 |
|---|---|
| `sampling.accepted`（整场累计） | 3100 |
| `trend.segment_accepted`（本段） | 2000 |
| `trend.window_sample_count`（窗口） | 2000 |
| `trend.truncated`（旧口径） | **true** ← 假话 |
| `trend.truncated`（应为） | **false** |

自相矛盾：覆盖过的环形序列必然满 2048，而这里有 2000。
**"杯子装满了 2048 毫升，现在里面有 2000 毫升。"**

从没被看见的原因不是它不常发生，是**从来没在 seek 之后取过 sync 报告**：
33 份带 `trend` 的 sync 证据里 32 份 `segments == 1`，第 33 份是本轮。
P12-F1 的 seek 取证走 `decode/` 那条线，不经过节拍器序列。

### B. 分位数与趋势是两批样本，却并排摆着

`quantiles.<口径>` 整场累计（覆盖后继续加、seek 后也继续加），
`trend` 只有窗口（封顶 2048 点）。两份都说了实话，摆法让人当同一批读。

r1 同一份数据：

| 口径 | 样本 | p50 | p95 | p99 | max |
|---|---|---|---|---|---|
| 整场累计 | 2600 | 0.125 | 0.125 | 0.375 | **6.518 ms** |
| 窗口 | 2048 | 0.125 | 0.125 | 0.375 | **2.040 ms** |

p50/p95/p99 **恰好相同**，只有 `max` 差 3.2 倍 —— 最差那一下落在被覆盖的头 552 个点里。
"恰好相同"是最坏的情况：它让两份数据看起来同源。

---

## 二、改法

- `DriftSeries` 新增 `segment_total_`（**唯一**写入点：`clear()` 归零 + 接受时 `++`），
  `truncated()` 改判 `segment_total_ > kCapacity`，新增 `segment_samples()`。
  归零放在 `clear()` 是为了让 `size() == min(segment_samples(), kCapacity)`
  **在任何时刻**成立（包括 clear 之后还没进新帧的那一瞬）。
- 新增单元 `core/pace/pace_drift_window.{hpp,cxx}`：`summarize_window(series)` 把窗口里
  还留着的点重放成一份分布，挂在 `PaceSummary::window_lateness`，渲染成 `trend.window_quantiles`。
  单开文件是因为 `pace_drift_trend.cxx` 是"回归"、这个是"分布"，混了之后
  `DriftTrend`（头文件写着"刻意压到 ~400 字节"）迟早被塞进一个 8 KB 直方图。
- `segment_samples` 一删为二：`segment_accepted`（段口径）+ `window_sample_count`（窗口口径）。
  旧名字两个意思都用过，而这两个数在 r2 上相差 1100。
- schema 升 `/7`。报告层的 `window_quantiles` **不写 `worst`**：累计那份已有，
  再给窗口一个同名不同口径的就是 P13-E 花一整张卡消灭的东西。

---

## 三、三轮实测

| 轮次 | 片源 | 帧 / 丢 / xrun | accepted | 段内 | 窗口 | truncated | segments |
|---|---|---|---|---|---|---|---|
| r1 | 130 s 360p30 | 3900 / 0 / 0 | 2600 | 2600 | 2048 | **true** | 1 |
| r2 | 130 s 360p30 + seek | 3000 / 0 / 0 | 3100 | 2000 | 2000 | **false** | **2** |
| r3 | 35 s 1080p30 | 1050 / 0 / 0 | 700 | 700 | 700 | false | 1 |

- r1：`media_span` 129.967 s、`observed_hz` 20.005；窗口满 2048，两份分布 max 差 3.2×。
- r2：`media_span` **154.933 s**（两段各算各的再加起来）、`observed_hz` 20.009、
  `unobserved_cells` 0；H1 的上界顶格：`3100 + 0 = floor(154.933/0.05) + 2 = 3100`。
- r3：`window_quantiles.histogram` 与 `quantiles.media_time_decimated.histogram`
  **逐桶完全相同** —— 一份是播放时逐帧累加的，一份是停机时从 2048 点序列重放的。
  两条独立路径给出同一份分布，说明序列真的留住了那批点。

判据 I0–I6 三轮 **全绿**；其中 r3 走的是 I4 的"逐桶相同"分支，
r1 走"覆盖所以可以不同"，r2 走"跨段所以可以不同"。三个分支各被一份真机证据踩到过 ——
**只做 r1 会以为窗口永远等于整场，只做 r2 会以为永远不等于。**

---

## 四、变异（C++ 3 处、Python 8 处、端到端 1 处，全部按预期打红）

**C++**

| 变异 | 结果 |
|---|---|
| C1 `truncated()` 退回 `total_ > kCapacity` | `t02` 1 条红（就是本卡的核心断言） |
| C2 `segment_total_` 把被抽稀门拒掉的帧也数进去 | 5 条红（`t02` / `t03` / `t09` / `t10` ×2） |
| C3 `summarize_window` 的 `samples` 取 `total_samples()` | 4 条红（`t08` / `t09` / `t10` ×2） |

**Python**（8 个样本，逐个植入后跑校验器）

| 变异 | 打红的判据 |
|---|---|
| M1 seek 轮写回 `truncated: true`（= 旧 C++ 的产出） | **I3 + I5** |
| M2 `segment_accepted` 写成整场 3100 | I2 + I3 + I5 |
| M3 `window_sample_count` 写成 2048（假装环满是满的） | I1 + I2 + I4 |
| M4 35 s 轮把一个桶挪 1 个样本 | **I4 逐桶比对**（报出"桶 512：661 vs 662"） |
| M5 删掉 `window_quantiles` | I4 存在性 |
| M6 `window_quantiles.samples` 写成整场 2600 | I4 样本数 |
| M7 `capacity` 写成 1024 | I0 常量 + I2 + I5 |
| M8 把旧的 `segment_samples` 加回去 | I0 命名 |

M1 是"把旧实现的产出直接喂给新判据"，也就是本卡最强的单点证明：
**同一份数据，旧代码写 true，新判据当场判它假。**

**端到端**：把 M1 拷成 `docs/evidence/p13i/p13i-mutation-bad-sync.json` 跑总收口 →
层 4 红、结论"不可交付"。并且**先断言了它确实进了 `docs/evidence/p13*/*-sync.json`
的 glob**（P13-F 的教训：文件名不匹配就没被扫到，总收口照样全绿）。

---

## 五、桌面与总收口

- pace 家族 8 个测试全过（新增 `kivo_p13i_window_tests`，64 断言）。
- 全量 `ctest`：263/271 通过，**8 个 `Not Run`**（可执行文件不在）。
  它们全部落在两处**既有**构建失败的下游：`ffmpeg_adapter.cpp`（`AVCodecContext` 未声明）
  与 `source_core/.../evidence_tests.cpp`（`SourceEvidenceItem` 初始化列表）。
  两个文件的 `git status` 干净、本卡一行没碰；P13-H 收口时也是这两处。
- 总收口 **348 份 / 四层通过 / 8 子校验器 exit 0 / 53 豁免**。
- APK 重建通过；三轮证据 `schema` 全 `/7`。

---

## 六、诚实清单

1. **判据自己犯了两次它要抓的错，两次都是"把整场当成一段"。**
   - H2 的上界原来只加了一次首帧的 `+1`，seek 轮报 20.0086 Hz 对上界 20.0065 Hz，
     被判成"分子分母不同源" —— 而数据是对的，`segments = 2` 就该有两个 `+1`。
   - I5 第一版写"未覆盖 ⇒ 整场不超过容量"，seek 轮当场假红：整场 3100 > 2048，
     可一个点都没被覆盖。覆盖只能在段内发生。
   两条都是假红。**假红比漏报更快地训练人忽略告警**，所以两条都改到位而不是放宽。
   值得记的是：写这两条判据的时候，脑子里想的正是"覆盖 ≠ 跨段"。
2. **这一轮的 p50/p95/p99 两份恰好相同，只有 max 不同。** 本卡不会让任何一张
   已有分位数表变样；它改的是"读者能不能发现两份不同源"。**恰好相同不是同源。**
3. **`truncated` 是语义变更，不是纯新增。** `/6` 及以前在 `segments > 1` 时给错答案。
   历史证据没有一份触发过（32 份全单段），所以不需要重读旧证据 —— 但那是运气。
4. **只有 1 轮 seek，只 seek 一次。** 三段以上、连续 seek、seek 之后又倒回去，
   都没真机覆盖；多次 `clear()` 下的计数归零只有单测 `t03`/`t04` 钉着。
5. **`PaceSummary` 大了约 8 KB**（多一个 `SignedHistogram`）。只在 stop 之后算一次，
   稳态零开销，但"整块复制的结构"确实变大了。
6. **`window_quantiles` 没有 `worst`**，是刻意的（见 §二）。代价是窗口口径说不出
   "这段最差那一下在哪儿"；要那个信息只能回累计那份看 `worst.media_ns`，
   再与 `trend.window_start_ns[0]` 比大小，自己判断在不在窗口内。
7. **窗口分布只做了 360p / 1080p 两种片源。** 4K 没跑：本卡量的是口径，
   不是解码能力，选便宜片源是刻意的 —— 但这也意味着**它不支持任何关于 4K 的结论**。
8. **漂移量准仍是 7 ppm 的尺子。** 三轮 `slope_from_windows_ppm` 全 0.000、`r2 ≈ 0`。
   这条从 P13-D 欠到现在，本卡一个字没动 —— 它是下一张卡最该做的事。

# P16-P1B10 · 时钟频差估计器（纯逻辑 + 行为证据）

- 日期：2026-11-06
- 定位：`183` §五 家族树里预留的 `clock_rate_estimator`，先做**测量层**，
  不接控制律、不出版证据。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`183-p16-p1b-audio-core-design.md` §四（方案 C：只加观测）、`184`（频差账目）、
  `03-goals.md` G7/G10。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**（不写、不复制源码）；无新目录、无项目包。

---

## 一、构思

`184` 用 `playback_rate` 这个**第三方对照量**证明了"门漏事件"（6/23 自然事件轮 `rebases=0`），
但它不是我们要的观测量：它是事后由墙钟跨度算出来的，不能逐帧喂给控制律。

要换速率伺服（`183` 方案 B），先得有一个**逐帧可用的频差估计**：
`r = Δposition / Δwall`，低通后得到 ppm 与相位误差。这是纯数学，与"该不该用 PLL"无关 ——
控制律要等评审，估计器不用。

所以本卡只做估计器 + 行为测试，**不接线、不出版、不改任何判定**。

---

## 二、规划设计

1. **纯逻辑、零依赖**：只吃 `(mono_ns, position_ns)` 样本，不读时钟、不分配、不加锁；
   放在 `core/audio/clock/`（该目录已是"纯数学 + 可桌面测"的族）。
2. **输出三件事实**：`raw_ppm`（本次瞬时）、`rate_ppm`（一阶低通）、
   `phase_error_ns`（位置 − 首样本位置 − 墙钟增量，与 `ClockPositionTracker` 的派生 lead 同构）。
3. **不可判时必须带非空 reason**（G9）：首个样本、墙钟增量 ≤ 0 都返回 `valid=false` + reason。
4. **不接线**：`ClockPositionTracker` 一行不改 —— 出版会碰 `pace_stats.*`（他人主线），
   本卡明确留到让出后。
5. **行为测试复用既有脚本**：`run_p16_burst_behavior_test.py` 的 SOURCES 加一行、
   CPP_SOURCE 加 T7–T9，不新增测试脚本。
6. 只操作 `dd137cf7`；`b6e950ee` 不碰。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/193-p16-p1b10-rate-estimator-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `183` 全文 |
| `core/audio/clock/clock_rate_estimator.hpp` | **新增** | 估计器（header-only：契约 + 内联实现） | 不读时钟、不分配、不含判定；**不新增 `.cxx`** |
| `tools/run_p16_burst_behavior_test.py` | 改 | CPP_SOURCE 加 T7–T9 | 不改 T1–T6、不改白名单、不改 SOURCES |

新增 **2** 文件、改 **1** 个既有文件；**无新目录**（都在既有 `core/audio/clock/` 与 `tools/`）。

**为什么 header-only（实现期更正）**：初版写了 `.hpp + .cxx`，但新增 `.cxx` 必须同时注册到两份
CMake 清单，而 `backend/CMakeLists.txt` 正被他人主线占用；不注册就会被
`verify_source_manifest.py` 判成"未注册源文件"，只能靠豁免放行。
纯数学、每帧一次、实现会被内联，不值得为此欠一条豁免 —— 于是收成 header-only。

### 判据（T7–T9）

| 判据 | 内容 |
| --- | --- |
| T7 | 喂 300 个"位置每步比墙钟快 0.5%"的样本：`rate_ppm` 收敛到 **5000 ± 50 ppm**，`phase_error_ns` 单调增大 |
| T8 | 喂 300 个 1.000 的样本：`rate_ppm` 落在 **±10 ppm**，`phase_error_ns` 绝对值 < 1 ms |
| T9 | `reset()` 后 `valid=false` 且 reason 非空；重新喂样本能再次收敛 |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | `--compile-only` | 9 个 TU 编译 0 错 |
| L2 | `--device dd137cf7` | T1–T9 **十八项断言全 PASS**（`RESULT: PASS`） |
| L3 | 六个门禁 self-test + 正式跑 | 全 PASS；CI 7 步 |

**结论边界**：这只证明"估计器在合成输入下能估出频差"，**不证明**它能替代漂移门，
也不证明真机负载下的收敛速度 —— 那要等接线 + 8 路负载 ≥8 轮，属 `183` 方案 B 的实现卡。

---

## 五、收口（2026-11-06）

### 交付

新增 **2** 个文件（`193` 文档、`core/audio/clock/clock_rate_estimator.hpp` header-only），
改 **1** 个既有文件（`run_p16_burst_behavior_test.py` 的 C++ 模板加 T7–T9）。
**无新目录、无未注册源文件、零 schema 变更。**

### 行为证据（`dd137cf7`，9 个 TU）

```
T7a rate_ppm converges to ~5000                PASS
T7b phase error positive when clock is fast    PASS
T8a rate_ppm ~= 0 for a steady clock           PASS
T8b phase error bounded (<1ms)                 PASS
T9a reset -> invalid with non-empty reason     PASS
T9b reset clears sample count                  PASS
RESULT: PASS (failures=0)      // 共 18 项断言（含既有 T1–T6）
```

即：合成输入下，0.5% 的频差被估成 **~5000 ppm**（容差 ±50），稳态钟落在 **±10 ppm**，
相位误差与 `ClockPositionTracker` 的派生 lead 同式，`reset()` 后回到"不可判 + 非空 reason"。

### 结论与边界

- 这只证明**估计器本身**在合成输入下能工作；**不证明**它能替代漂移门，
  也不证明真机负载下的收敛速度。
- 估计器**未接线**：`ClockPositionTracker` 一行未改，证据里不会出现新字段。
  接线要碰 `pace_stats.*`（他人主线），本卡明确留到让出后。
- 控制律（是否用速率伺服、如何与 jump/stale 分支共存）仍等 `183` §七 Q1/Q2；
  本卡不越这条门闸。

### 未做

不接线、不出版、不改 `PacePolicy`、不碰 `pace_stats.*` / `av_sync_report.cxx`、
不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不接线 `ClockPositionTracker`、不出版证据、不改 `PacePolicy`、不改任何判定；
- 不碰 `pace_stats.*` / `av_sync_report.cxx` 等他人主线；
- 不实现伺服控制律（等 `183` §七 Q1/Q2）；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`。

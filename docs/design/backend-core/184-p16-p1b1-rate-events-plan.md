# P16-P1B1 · 频差事件证据重算（换控制律前的判据基线）

- 日期：2026-11-06
- 定位：本卡是 `183-p16-p1b-audio-core-design.md` §二留下的前置任务。**只重算真机证据，不写实现代码、不改控制律、不调阈值、不动证据文件。**
- 门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`183` §二（"实现卡必须先用既有 `tools/verify_*` 重算这些数字，否则不许宣称复现"）、`03-goals.md` G10。
- 工作目录 `C:\kivo video`，无新目录、无项目包；`F:\kivo` 仍是只读参照，不在其中写文件。

---

## 一、构思

`183` 里 A1 的数字（8 秒 118.79 ms / 1.49%、P13-AB `rebases=5/6/10`、总修正 35.2 ms）
全部转述自**源码注释与任务卡**，不是本轮从证据重算的。
换控制律之前必须有一份能被第三方重算的账：到底哪些轮真的出现频差、门在其中哪些轮没响、
注入轮的收敛残差是多少。否则 B 阶段（速率伺服）无法做 A/B，"更快/更慢"全靠嘴说。

### 实测发现：字段名与文档叫法不一致（保留，别再按文档猜）

| 文档/注释里的叫法 | 证据里的真实路径 |
| --- | --- |
| `clock_ahead` | **不存在这个键**；等价量在 `av_drift.trend.drift_accumulated_ns`、`timeline.lead_correction_ns` |
| `playback_rate` | `timeline.playback_rate` |
| `drift_rebases` | `pace.clock_health.drift_rebases` |
| 注入参数 | `clock_injection.offset_ns / arm_after_ns / armed_at_position_ns / rate_ppm` |
| 门开关 | `drift_gate.enabled / limit_ns / band_ns` |

`docs/evidence/` 共 **811** 份 JSON，路径含 `p13` 的 **347** 份；含 `timeline.playback_rate` 的 **162** 份。
本卡只以这 162 份为账目范围。

---

## 二、规划设计

1. **只读**：脚本不写任何文件，不修改证据，不 import 项目代码。
2. **判据是结构性不变量，不是"数字好不好看"**：不同阶段卡（AB/AC/AC2/AC3/AD）口径本就不同，
   给它们设统一阈值会把历史证据误判成红。只钉住"账目自洽"这类不变量。
3. **门沉默必须显式列出**，不许用汇总数字盖掉 —— 呼应项目铁律"豁免必须明示，
   靠隐藏换来的全绿比红更危险"。
4. **不碰他人主线**：不读改 `docs/handoff/*`、`backend/CMakeLists.txt`、`verify_p12_*` 等被占用的文件；
   只读 `docs/evidence/p13*`。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/184-p16-p1b1-rate-events-plan.md` | 新增 | 本卡规划、字段映射、判据、验收、收口 | 不复制 `183` 全文 |
| `tools/verify_p16_rate_events.py` | **新增** | 从 P13 sync 证据重算频差事件账目 + 变异自检 | 不判断控制律好坏、不改证据、不替代 `verify_p14a_metrics.py` |
| `.github/workflows/p2-ci-build-test.yml` | 改 | `hard-gates` 增加该校验器一步 | 不动其它 job、不加 `continue-on-error` |

新增 2 个文件、改 1 个既有文件，全部归位既有目录（`docs/design/backend-core/`、`tools/`）；**无新目录**。

### 五条判据（R1–R5，全部可复算）

| 判据 | 内容 |
| --- | --- |
| R1 字段齐备 | 有 `timeline.playback_rate` 的证据必须有 `pace.clock_health.drift_rebases`，否则账目不可判 |
| R2 计数与修正账一致 | `drift_rebases > 0` ⇒ `drift_corrected_total_ns > 0`；反之 `= 0` 时修正总量必须为 0 |
| R3 对照轮语义 | `drift_gate.enabled == false` 的轮次 `drift_rebases` 必须为 0，且脚本单列"对照组"，不混入"门沉默" |
| R4 注入生效 | `clock_injection.offset_ns > 0` ⇒ `armed_at_position_ns >= 0`；给了参数却没注入必须显式报出 |
| R5 事件分类 | `abs(playback_rate − 1) >= 0.005` 为事件轮，分「注入轮 / 自然轮」两组；自然轮里 `drift_rebases == 0` 的逐份列出（门没响） |

### 变异自检

对 R1–R4 各注入一处破坏（删字段、清零计数、给对照轮加计数、把注入轮的 armed 改成 −1），
必须被抓到；抓不到即校验器失效（退出码 2）。

### 实现期更正：R1 不判红（保留上表原文）

上表 R1 写「必须有 `pace.clock_health.drift_rebases`，否则账目不可判」—— 照字面判红会让
162 份里的 **90** 份早期证据全部失败；那不是账目不自洽，是 schema 演进（AB 起才出计数、
AC 起才出 `drift_corrected_total_ns`）。按 P13-E 的既有口径更正为：
**缺字段 = `not_measured`，单列打印、不参与账目、不冒充门沉默**；R5 的"门沉默"只统计
"计数确实出版且为 0"的轮次。这不是放宽判据，是把「不可判」与「沉默」分开。

---

## 四、验收

- `python tools/verify_p16_rate_events.py --self-test` → 变异全 CAUGHT；
- `python tools/verify_p16_rate_events.py` → 打印账目表，退出码 0 表示账目自洽；
- 重算结论写回本文「五、收口」；`183` §二的"数值口径"补更正块（保留原文）。
- **本卡不产出"控制律该换"的结论**，只产出可复算的基线数字；是否换伺服仍等 `183` §七 的评审答复。

---

## 五、收口（2026-11-06）

### 账目重算结果（`python tools/verify_p16_rate_events.py`）

| 项 | 数字 |
| --- | --- |
| 含 `timeline.playback_rate` 的 P13 证据 | **162** 份 |
| 缺 `drift_rebases` → `not_measured` | **90** 份（早期卡，P13-C/B/D 等） |
| `drift_gate.enabled=false` 对照轮 | **3** 份 |
| 事件轮 `abs(rate−1) >= 0.005` | **40** 份（注入 17 / 自然 23） |
| 事件轮 rate 范围 | **0.624921 .. 1.056475** |
| `drift_rebases` 合计 / 单轮最大 | **632 / 117** |
| **自然事件轮里门没响** | **6 份**（逐份列出，见下） |

门没响的 6 轮（无注入、非对照、`rebases=0`）：

| 证据 | playback_rate |
| --- | --- |
| `p13s/p13s-4k60-r1-sync.json` | **1.032032** |
| `p13u/p13u-8x-r3-sync.json` | 1.011632 |
| `p13v/p13v-8x-r2-sync.json` | 1.021176 |
| `p13x/p13x-8x-r2-sync.json` | 1.014671 |
| `p13z/p13z-8x-r1-sync.json` | 1.005868 |
| `p13z/p13z-8x-r4-sync.json` | 1.009277 |

### 对 `183` A1 判断的影响

- "门反复改判据仍漏事件"不再是转述：**6 份自然轮逐份可复算**，其中 `p13s-4k60-r1` 快 **3.2%** 门仍沉默。
- 注入侧演进链：P13-AB **2/5/6/10** → AC **18/78** → AC2 **21/81** → AC3 **1**；
  `p13ae-r5000-on-r1` 达 **117 次复位 / 597 ms 修正量**。这是 bang-bang 在持续频差下反复动作的直接证据，
  也正是 B 方案（速率伺服）要消灭的形状。
- `p13x-8x-r1` rate **0.995868**（慢 0.4%）对应 P13-X 的"复位过冲"，同样在账里。

### 已知口径问题（留给 B 阶段）

事件轮 rate 下界出现 **0.624921**，明显不是频差，而是丢帧 / 量程导致的第三方对照失真。
本卡按"账目自洽"放行、不判红；**B 阶段做 A/B 前必须先给 `playback_rate` 加适用条件**
（丢帧轮单列 `not_measured`），否则会把异常轮误读成伺服效果。

### 验证

- `--self-test`：5 个变异全 CAUGHT；正式跑 exit 0，R1–R5 账目自洽。
- CI `hard-gates` 已接入（现 4 步，无 `continue-on-error`）。
- 本卡未写实现代码、未调阈值、未改任何证据、未碰他人主线文件、未在 `F:\kivo` 写文件。

---

## 六、明确不做

- 不实现 `clock_rate_estimator` / `audio_drain_ticket`（`183` §五 的实现卡，需评审结论）；
- 不改 `PacePolicy` 任何阈值、不动 `clock_position_tracker.cxx`、不改证据 schema；
- 不做真机新轮次（B 阶段再开卡，需 8 路负载 ≥8 轮）；
- 不碰他人主线文件，不在 `F:\kivo` 写文件。

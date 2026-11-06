# P16-P1B5 · 采样率切换的位置连续性行为证据

- 日期：2026-11-06
- 定位：把 `183` §七 **Q5**（设备周期/采样率切换时是否重建流）里**属于我方事实**的那一半先钉死。
  门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`186-p16-p1b3-behavior-evidence-plan.md`、`183-p16-p1b-audio-core-design.md` §七、
  `03-goals.md` G7/G10。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照；无新目录、无项目包。

---

## 一、构思

Q5 问的是"设备周期/采样率切换时你们是否重建流"。我方答案其实已经写在代码里：
**不重建** —— `aaudio_clock_source.cxx:140-153` 的 `set_sample_rate()` 原地改速率、重算
`stale_after_ns_`，`FramePositionExtrapolator` 承诺"位置连续"；这与
`F:\kivo` 的 `M1-GAPLESS-ASSESSMENT.md:45`「必须复用同一条 AAudio 流」同向。

但这份承诺**一次都没被测过**。而且 `frame_position_extrapolator.hpp:39-44` 写了一个非常具体的
易错点：

> 48000 帧在 48k 下是 1000 ms，切到 44.1k 就成 1088 ms，直接跳 88 ms。
> 因此切换时把基准挪到"最后已知帧位置"上。

88 ms 足以越过 G7 的 40 ms 阈值，却小到不会触发任何跳变守卫
（`PacePolicy::jump_limit_ns = 1s`）。这类"守卫抓不到、证据看不出来"的缺陷，
正是应该先用行为测试钉住的。等对方回答 Q5 时，两边就都有事实而不是猜测。

---

## 二、规划设计

1. **不改任何产品代码**：本卡只扩 `186` 的行为测试 TU，加 T4–T6 三条断言。
2. **不新增仓库目录/文件**（除设计稿）：断言内嵌在既有 `run_p16_burst_behavior_test.py`
   的 C++ 模板里，产物仍落 `build/p16-burst-behavior/`。
3. **可控时钟**：注入静态 `now_fn`（`g_now_ns` 固定）与返回 `kOk` 时间戳的 fake probe，
   让"同一墙钟时刻切速率"成为确定性场景；不用 sleep、不碰真实 AAudio。
4. **不碰他人主线**：`backend/CMakeLists.txt`、`pace_stats.*`、`av_sync_report.cxx` 等一律不动。
5. **不冒充回答 Q5**：本卡只给出"我方切换后位置连续"的事实；
   对方是否重建流、以及设备周期变化时的边界条件，仍等 `183` §七 答复。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/188-p16-p1b5-sample-rate-design.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `183/186` 全文 |
| `tools/run_p16_burst_behavior_test.py` | 改 | 在既有测试 TU 增加 T4–T6 断言 | 不改 T1–T3、不改设备白名单、不改 SDK 三级解析 |

新增 **1** 个文件、改 **1** 个既有文件；**无新目录、无新体系**。

### 判据（T4–T6）

| 判据 | 内容 |
| --- | --- |
| T4 | `set_sample_rate(44100)` 后 `stats().sample_rate == 44100`；`set_sample_rate(0)` 不改变它（防御） |
| T5 | 兜底实例切换采样率后 `used_fallback()` 仍为 true、`fallback_reason()` 仍非空、`now()` 不崩 |
| T6 | 同一墙钟时刻、同一帧位置（48000 帧）下把速率从 48k 切到 44.1k，前后 `position_ns` 差 **< 1 ms**（若基准没挪应跳 88 ms，必须被抓到） |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | `--self-check` + `--compile-only` | SDK 解析 PASS；9 个 TU 编译 0 错 |
| L2 | `--device dd137cf7` | T1–T6 **十二项断言全 PASS**（`RESULT: PASS failures=0`） |
| L3 | 五个门禁 self-test + 正式跑；CI `hard-gates` 6 步 | 全 PASS、无 `continue-on-error` |

**设备纪律**：只操作 `dd137cf7`；`b6e950ee` 不碰；不 `uninstall`；临时可执行跑完清理。

---

## 五、收口（2026-11-06）

### 交付

新增 **1** 个文件（`188` 文档），改 **1** 个既有文件
（`tools/run_p16_burst_behavior_test.py` 的 C++ 模板加 T4–T6）。
**无新目录、无新体系、不改任何产品代码。**

### 行为证据（`dd137cf7`，NDK 30 交叉编译 9 个 TU）

`186` 既有的 T1a–T3b 六项全 PASS；本卡新增六项：

```
T4a set_sample_rate(44100) applied             PASS
T4b set_sample_rate(0) is a no-op              PASS
T5a fallback survives rate switch              PASS
T5b fallback reason survives rate switch       PASS
T5c fallback now() after rate switch           PASS
T6 position delta after 48k->44.1k: 0 ns
T6 no 88ms jump at fixed wall clock (<1ms)     PASS
RESULT: PASS (failures=0)      // 共 12 项断言
```

**结论**：我方 `set_sample_rate()` 在同一墙钟时刻、同一帧位置下不制造 88 ms 跳变，
位置连续；`frame_position_extrapolator.hpp:39-44` 承诺的"把基准挪到最后已知帧位置"成立。
如实说明：这是**固定墙钟 + fake probe 的确定性换算验证**，验的是契约，
不是真实设备切换采样率的端到端验证。

### 对 Q5 的作用

Q5 问我方是否重建流 → 答案是**不重建**（原地 `set_sample_rate()` + 重算 `stale_after_ns_`），
且现在有行为证据证明切换不会让位置跳变。`F:\kivo` 的
`M1-GAPLESS-ASSESSMENT.md:45`「必须复用同一条 AAudio 流」与我方同向。
**对方是否重建流、设备周期变化时的边界条件仍等 `183` §七 答复** —— 本卡不外推。

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | `--self-check` PASS（`source=android/local.properties`）；`--compile-only` 9 TU OK |
| L2 | `dd137cf7` 12 项断言全 PASS |
| L3 | 五个门禁 self-test + 正式跑全 PASS；CI `hard-gates` 6 步、无 `continue-on-error` |

### 未做

不改产品代码；不回答 Q5 对方那一半；不实现 `183` §五 的实现卡；
不改证据 schema、不调阈值、不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不改 `AaudioClockSource` / `FramePositionExtrapolator` 的任何实现；
- 不实现 `183` §五 的实现卡（`clock_rate_estimator` / `audio_drain_ticket`）；
- 不回答 Q5 里"对方是否重建流"的那一半，不把本卡结论外推成跨设备结论；
- 不改证据 schema、不调阈值、不碰 `F:\kivo`、不新建项目包。

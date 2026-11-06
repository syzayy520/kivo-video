# P16-P1B3 · 兜底行为证据与失败链门禁加固

- 日期：2026-11-06
- 定位：本卡收掉 `185` 留下的两处"只证形状、未证行为"。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`185-p16-p1b2-burst-g9-plan.md`、`03-goals.md` G9/G10、`08-p12-family-tree.md`。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照，不在其中写文件；**无新目录、无项目包**。

---

## 一、构思

`185` 的 B1–B5 是**静态文本门禁**：它们能证明"拒绝分支的字面还在"，
但证明不了两件事：

1. `AaudioClockSource(..., frames_per_burst=0, ...)` 构造后 `used_fallback()` 真的返回 `true`、
   `fallback_reason()` 真的非空；传 192 时真的返回 `false`。这个行为一次都没跑过。
2. 失败链只钉到 `AaudioOutputSession::open()` 的 `code+message`，中间
   `AudioPlayback::open():94` 的 `stats_.error = "aaudio open: " + opened.message`
   与下游 `av_sync_report.cxx` 的 `audio.error / completed` 没有被门禁覆盖 ——
   中间任一环被删，reason 就到不了用户眼前，而 B1–B5 仍然全绿。

---

## 二、规划设计

1. **行为测试独立编译、不注册 ctest**：`backend/CMakeLists.txt` 正被他人主线占用，
   本卡不碰它。用 NDK clang 直接把 `aaudio_clock_source.cxx` + 所需 core `.cxx`
   交叉编译成 aarch64 可执行，push 到 `dd137cf7` 运行并断言。
2. **测试源内嵌在运行脚本里**，产物落在既有 `build/`（构建产物目录）。
   理由：家族树 §一 第 2 条"子域文件数 ≥ 3 才开子目录"，单个测试 TU 不该新开目录；
   而放进别人的 `p12c4_clock_tests/` 又会把 P16 语义混进 P12 卡。内嵌 = 零新增仓库目录。
3. **fake probe 是纯逻辑**：`AaudioClockProbe` 是零平台头的纯虚接口
   （`aaudio_clock_probe.hpp:16-23`），实现一个返回固定 reading 的 fake 即可，不碰 AAudio。
4. **失败链判据是"接线声明"**：`av_sync_report.cxx` 正在被他人修改，本卡**只读**它；
   若对方重构了 `audio.error` 的写法，门禁会红 —— 这是有意的，
   但收口里必须写明"报表重构时需同步更新 B6"，免得被误当成代码缺陷。
5. **只读证据、不改 schema、不调阈值、不进 `183` §五 的实现卡**（评审门闸仍关着）。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/186-p16-p1b3-behavior-evidence-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `183/185` 全文 |
| `tools/run_p16_burst_behavior_test.py` | **新增** | 编译 + 设备运行 + 断言「兜底可查询」的真实行为 | 不改任何仓库源文件、不产证据 JSON、不替代 ctest |
| `tools/verify_p16_burst_gate.py` | 改 | 增加 B6「失败链完整」判据与 M6 变异 | 不改 B1–B5 既有语义 |
| `.github/workflows/p2-ci-build-test.yml` | 改 | `hard-gates` 增加行为测试入口？**否** —— CI 无设备，见验收节 | 不把设备依赖塞进 CI |

新增 **2** 个文件、改 **1** 个既有文件；测试产物在既有 `build/`；**无新目录**。

> CI 接入的取舍：行为测试需要 adb + 真机，`hard-gates` 跑在 ubuntu-latest 上没有设备，
> 硬接会让 CI 永久红。本卡把它作为**本地可复算的设备侧脚本**，CI 仍由 B1–B6 的静态门禁守护。
> 这一点如实写进收口，不冒充"CI 也跑了行为测试"。

---

## 四、判据

### 行为判据（T1–T3，脚本内断言）

| 判据 | 内容 |
| --- | --- |
| T1 | 传 `frames_per_burst=192`：`used_fallback()==false`、`fallback_reason()=="device"` |
| T2 | 传 `0`：`used_fallback()==true`、`fallback_reason()` 非空且含 `unavailable` |
| T3 | 传 `0` 时仍能安全算出正陈旧阈值：`now()` 可调用、不崩、`stats().sample_rate>0` |

### 静态判据（B6，加到 `verify_p16_burst_gate.py`）

| 判据 | 内容 |
| --- | --- |
| B6 | `audio_playback.cxx` 保留 `stats_.error = "aaudio open: " + opened.message` 拼接；`av_sync_report.cxx` 仍出现 `audio.error` 与 `completed` |

变异自检 M6：把 `"aaudio open: "` 拼接删掉 → 必须被抓到。

---

## 五、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | `python tools/verify_p16_burst_gate.py --self-test` + 正式跑 | 6 条变异（M1–M6）全 CAUGHT、正式 PASS |
| L1 | 四个校验器全量复跑 | self-test + 正式全 PASS；CI `hard-gates` 5 步不变 |
| L2 | `python tools/run_p16_burst_behavior_test.py --device dd137cf7` | NDK 交叉编译 0 错；设备上 T1/T2/T3 全 PASS；临时文件清理 |
| L3 | 不重跑播放 | 本卡不改播放路径行为，`185` 的真机 A/B 结论继续有效 |

**设备纪律**：只操作 `dd137cf7`；`b6e950ee` 不碰；不 `uninstall`；push 的可执行与临时源码在
`/data/local/tmp` 与 `build/` 下，跑完清理；应用私有证据保留。

---

## 六、收口（2026-11-06）

### 交付

新增 **2** 个文件（`186` 文档、`tools/run_p16_burst_behavior_test.py` 187 行），
改 **1** 个既有文件（`tools/verify_p16_burst_gate.py` 185 行）。无新目录、无项目包；
测试源内嵌、产物落既有 `build/p16-burst-behavior/`。CI `hard-gates` 保持 **5 步**
（行为测试不进 CI，理由见 §三）。

### 行为证据（`dd137cf7`）

NDK 30 交叉编译 **9 个 TU** 成功；设备运行输出：

```
T1a device(192) used_fallback == false     PASS
T1b device(192) reason == device           PASS
T2a fallback(0) used_fallback == true      PASS
T2b fallback(0) reason nonempty + explicit PASS
T3a fallback sample_rate > 0               PASS
T3b fallback reason differs from device    PASS
RESULT: PASS (failures=0)
```

白名单验证：`--device b6e950ee` 被拒绝（exit 2 + FATAL），**主设备全程未碰**；
设备上临时可执行已 `rm -f` 清理，应用私有证据保留。

### 静态门禁

B6 新增：`audio_playback.cxx` 的 `"aaudio open: "` 拼接 + `av_sync_report.cxx` 的
`audio.error / completed` 仍在。变异 **M1–M6 全 CAUGHT**、baseline 0、正式 PASS；
四个门禁 self-test + 正式跑全 PASS；CI 5 步、无 `continue-on-error`。

### 如实说明

- 行为测试**没有注册 ctest**（`backend/CMakeLists.txt` 被他人主线占用），
  也**不进 CI**（ubuntu runner 无真机）。它是设备侧可复算脚本，不是自动化 CI 门禁 —— 不冒充。
- B6 只读 `av_sync_report.cxx`；若对方重构 `audio.error` 写法，B6 会红。
  这是**接线声明**，需同步更新判据，不是代码缺陷。
- 本卡不重跑播放（未改播放路径行为），`185` 的真机 A/B 结论继续有效。
- 负向分支仍无法在真机伪造（`getFramesPerBurst` 由平台返回）。本卡验的是
  "兜底状态可查询"这一**行为**，不是"平台返回 0 时开流失败"的完整设备链路。

### 未做

`183` §五 的实现卡（等 `183` §七 评审）、xrun 归因 / 音频焦点 / USB·蓝牙切换 / 第二 SoC 矩阵；
不改证据 schema、不调阈值、不碰 `F:\kivo`、不 `uninstall`。

---

## 七、明确不做

- 不把行为测试注册进 `backend/CMakeLists.txt`（他人主线）、不新建测试目录；
- 不把设备依赖塞进 CI（无真机 runner）；
- 不实现 `183` §五 的 `clock_rate_estimator` / `audio_drain_ticket`（等 `183` §七 评审）；
- 不改证据 schema、不调阈值、不碰 `av_sync_report.cxx` 的内容（只读）；
- 不在 `F:\kivo` 写文件，不新建项目包。

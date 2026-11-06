# P16-P1B2 · burst 兜底显式化（G9 静默降级收口）

- 日期：2026-11-06
- 定位：本卡做 `183` §二 A3 里**不依赖外部评审**的那一半。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`03-goals.md` G9、`182` §八 A3、`183` §二 A3 的更正块、`08-p12-family-tree.md`。
- 工作目录 `C:\kivo video`；`F:\kivo` 仍只读参照，不在其中写文件；无新目录、无项目包。

---

## 一、构思

`AAudioStream_getFramesPerBurst` 的真实值经
`aaudio_stream_opener.cxx:72` → `aaudio_output_session.cxx:63` 传给 `AaudioClockSource`。
但当平台返回 `<= 0` 时，`aaudio_clock_source.cxx:25` 会悄悄退回 `kDefaultFramesPerBurst = 192`
（K40 在 48 kHz 下的 4 ms）。两件坏事同时发生：

1. **证据说瞎话**：`frames_per_burst` 在证据里看起来像"设备协商到 192"，与真协商到 192 无法区分；
2. **G7 会被带偏**：`stale_after_ns_ = 2 × burst / sample_rate`，用错 burst 会把"新鲜"误判成"陈旧"
   （`aaudio_clock_source.cxx:11-13`、头文件 `:29-30`），进而触发 FreeRun 降级。

G9 要求「任何能力缺失必须带非空 reason 且用户可见」。当前这条路径两者皆无。

### 为什么是"拒绝"而不是"标注后继续"

`AudioPlayback::open():101-115` 已有同一套约定：协商结果与片源不一致时**直接拒绝**
（"硬播只会得到音高错误的输出，而且听感上很难归因；本项目不做重采样"）。
burst 不是可选项，而是时钟陈旧阈值的输入，猜一个值继续播属于同一类错误。
所以复用既有失败通道，不改 schema、不新增判据。

---

## 二、规划设计

1. **正常路径逐位不变**：真机上 `getFramesPerBurst` 返回 192，新分支不执行；
   `audio.error` 仍为空、`completed` 仍为 true、证据字段一个不改。
2. **复用既有失败链**：`AaudioOpenOutcome.code=kOpenFailed` + 非空 `message`
   → `AudioPlayback::open():93-94` 的 `stats_.error = "aaudio open: " + message`
   → `av_sync_report.cxx:605` 的 `audio.error` / `completed=false`
   → JNI `last_error` → `FailureEvidence` → UI。**不新增 schema 字段、不碰他人正在改的报表文件。**
3. **类本身也不撒谎**：`AaudioClockSource` 保留兜底作为"其它调用方/单测传 0 时的自卫"
   （防止除零与负阈值），但必须能查询 `used_fallback()` 与 `fallback_reason()`；
   产品路径由上游拒绝，不会走到这里。
4. **不碰他人主线**：`backend/CMakeLists.txt`、`av_sync_report.cxx`、`pace_stats.*`、
   `docs/handoff/*`、`docs/evidence/*`、`tools/verify_p12_*` 一律不动。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/185-p16-p1b2-burst-g9-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `183/184` 全文 |
| `tools/verify_p16_burst_gate.py` | **新增** | 钉住"burst 兜底不回退为静默降级"+ 变异自检 | 不判断设备数值、不改证据、不替代其它校验器 |
| `backends/aaudio/session/aaudio_output_session.cxx` | 改 | 开流时拒绝无效 burst，写非空 reason | 不动开流其它分支、不改释放顺序 |
| `backends/aaudio/clock/aaudio_clock_source.{hpp,cxx}` | 改 | 让兜底可查询（`used_fallback` / `fallback_reason`） | 不改时钟读数与外推逻辑 |
| `.github/workflows/p2-ci-build-test.yml` | 改 | `hard-gates` 增加一步 | 不动其它 job、不加 `continue-on-error` |

新增 2 个文件、改 4 个既有文件，全部落在既有族；**无新目录**。

### 五条判据（B1–B5）

| 判据 | 内容 |
| --- | --- |
| B1 | `aaudio_output_session.cxx` 在构造时钟**之前**有 `frames_per_burst <= 0` 的显式拒绝分支 |
| B2 | 该分支设 `kOpenFailed` 且 `message` 含非空原因（含 `refusing to fall back` 与 `device burst`） |
| B3 | `aaudio_clock_source.hpp` 提供 `used_fallback()` / `fallback_reason()`；实现里记录该状态 |
| B4 | 兜底常量 192 仍在（自卫），但注释标明"产品路径已在上游拒绝" |
| B5 | CI `hard-gates` 含该校验器，且 `hard-gates` 段无 `continue-on-error` |

变异自检：删掉拒绝分支、清空 message、去掉 `used_fallback()`、给 CI 加回 `continue-on-error`，各自必须被抓到。

---

## 四、实现顺序

1. 本文落盘；
2. `aaudio_output_session.cxx` 拒绝分支 → `aaudio_clock_source.{hpp,cxx}` 可查询兜底；
3. `tools/verify_p16_burst_gate.py` → CI 接入；
4. 验证（下一节）；
5. 收口写回「五」。

---

## 五、验收口径

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | 四个校验器 `--self-test` + 正式跑 | 全 PASS；`hard-gates` 5 步、无 `continue-on-error` |
| L2 | NDK clang `-fsyntax-only` 改动的 3 个 TU；独立 CMake 双 flavor 增量构建 | 0 错 0 警；`libkivo_player.so` 链接成功 |
| L3 | `dd137cf7`：`assembleDebug` 增量 → `install -r`（**不 uninstall**）→ 标准参数起播一轮 | `audio.negotiated.frames_per_burst=192`、`audio.error=""`、`completed=true`、`error=""`；证明正常路径逐位不变 |

**负向分支无法在真机伪造**（`getFramesPerBurst` 由平台返回），因此 B1/B2 由静态门禁 + 编译验证守护，
真机只验"正常路径没被改坏"。这一点如实写在收口里，不冒充已验证失败路径。

---

## 六、收口（2026-11-06）

### 交付

新增 2 个文件（`185` 文档、`tools/verify_p16_burst_gate.py`），改 4 个既有文件
（`aaudio_output_session.cxx`、`aaudio_clock_source.{hpp,cxx}`、CI yml）；**无新目录、无项目包**。
CI `hard-gates` 现 **5 步**，仍无 `continue-on-error`。

### 实现要点

- **产品路径**：`AaudioOutputSession::open()` 在 `stream_` 赋值**之前**检查
  `frames_per_burst <= 0`，设 `kOpenFailed` + 非空 message，走既有
  `code+message → stats_.error → audio.error / completed=false → last_error → UI` 失败链。
  **不改任何 schema、不碰被他人占用的 `av_sync_report.cxx`。**
- **类本身**：`AaudioClockSource` 保留 192 自卫（防传 0 算出非正陈旧阈值），
  新增 `used_fallback()` / `fallback_reason()`；构造按声明顺序先记 `used_fallback_` 再算阈值。
  产品路径不会走到 `true`，兜底不再伪装成设备协商结果。

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | 4 个校验器 self-test + 正式跑全 PASS；新门禁 baseline 0、5 个变异全 CAUGHT；CI 5 步 |
| L2 | NDK clang `-fsyntax-only` **0 错 0 警**；独立 CMake **ON / OFF 双 flavor 各 13/13 步**、`libkivo_player.so` 链接成功；Gradle `assembleDebug` **BUILD SUCCESSFUL（40 s）**、`buildCMakeDebug[arm64-v8a]` 重编原生库 |
| L3 | `dd137cf7` 覆盖安装（`install -r`，**无 uninstall**）+ 标准参数起播一轮 |

### L3 真机 A/B（同一设备 / 同一片源 / 同一参数）

| 字段 | 修复前 `p16p1a-golden` | 修复后 `p16p1b2-golden` |
| --- | --- | --- |
| `outcome` | `completed=true / error=""` | `completed=true / error=""` |
| `frames_rendered / dropped_late` | **1050 / 0** | **1050 / 0** |
| `wall_us / speed_vs_realtime` | 35,030,603 / 0.999126 | 35,007,747 / 0.999779 |
| `alloc.hook_installed` | true | true |
| `sync audio.negotiated.frames_per_burst` | **192** | **192** |
| `sync audio.opened / started` | true / true | true / true |
| `sync completed / xrun / frames_dropped` | true / — | true / **0 / 0** |

`speed_vs_realtime` 的 0.999126 → 0.999779 属正常轮次抖动（偏差 < 0.1%），结构字段逐项一致。
**结论**：burst 来自设备真值 192，新拒绝分支没有执行，正常路径逐位不变。

### 如实说明

负向分支（`getFramesPerBurst` 返回 `<= 0`）**无法在真机伪造**——它由平台返回。
所以 B1/B2 只由静态门禁 + 编译验证守护，真机这一轮验的是"没把正常路径改坏"。
**不宣称失败路径已在设备上跑过。**

### 未做

`183` §五 的实现卡（等 `183` §七 评审）、xrun 归因 / 音频焦点 / USB·蓝牙切换 / 第二 SoC 矩阵；
不碰他人主线文件、不在 `F:\kivo` 写文件、不 `uninstall`、`b6e950ee` 未碰。

---

## 七、明确不做

- 不实现 `183` §五 的 `clock_rate_estimator` / `audio_drain_ticket`（等 `183` §七 评审结论）；
- 不改 `PacePolicy` 阈值、不动 `clock_position_tracker.cxx`、不改证据 schema；
- 不做 xrun 归因 / 音频焦点 / USB·蓝牙切换 / 第二 SoC 矩阵（A3 的其余部分另开卡）；
- 不碰他人主线文件，不在 `F:\kivo` 写文件。

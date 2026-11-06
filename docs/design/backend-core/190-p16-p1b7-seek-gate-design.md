# P16-P1B7 · A2 实现卡：seek 栅栏换成显式凭据

- 日期：2026-11-06
- 定位：`183` §二 A2 的实现卡。**Q3/Q4 不再等外部答复** —— 答案在用户提供的
  `F:\kivo` 参考核心里查到了（见 §一）。门闸：构思 → 规划设计 → 自然家族树（本文）→ 实现 → 验收。
- 上位：`189-p16-p1b6-seek-gate-observation-plan.md`（观测层）、`183` §七 Q3/Q4、
  `03-goals.md` G9、`08-p12-family-tree.md`。
- 工作目录 `C:\kivo video`；`F:\kivo` 只读参照，不在其中写文件；无新目录、无项目包。

---

## 一、构思：Q3/Q4 从参考核心查到的答案

| 问题 | 参考核心的事实 | 路径 |
| --- | --- | --- |
| **Q3 ticket 由谁发 complete** | **控制线程**。`drain()` 开头强制 `on_control_thread()`，否则返回 `wrong_thread_result()`；`complete_end_of_stream()` 在 runtime pump 里 `begin_drain → renderer_.drain → complete_drain` 一气呵成。音频回调线程**完全不写完成状态** | `wasapi_renderer_drain.cpp:42-44`、`playback_runtime_drain.cpp:8-55` |
| **Q4 超时怎么定** | **可配置整段预算**（默认 `std::chrono::seconds{2}`），`ceil_milliseconds` 向上取整、`saturating_tick_deadline` 防溢出，用 `WaitForSingleObject` 事件等待而非固定睡眠；超时独立分类 `RenderFailure::Timeout` 并计入 `drain_timeouts_` | `playback_runtime_coordinator_state.hpp:178-179`、`wasapi_renderer_drain.cpp:18-32,59-60,81-94` |
| Q5 是否重建流 | 不重建（复用同一条流） | `M1-GAPLESS-ASSESSMENT.md:45`；我方 `188` 已证位置连续 |

所以 `183` §七 里让我停住的两个"设计分歧"，参考核心里其实没有分歧：
**完成由控制线程判定，超时是带 deadline 的预算，且必须有独立分类**。
我此前担心的"让实时回调线程写完成状态"是伪问题 —— 对方根本不这么做。

### 现状与实测基础

`audio_playback.cxx:210-212` 靠 `sleep_for(10ms)` 猜在途回调已退出。
`189` 在 `dd137cf7` 实测：`render()` 临界区 p50 1 µs / p99 2 µs / max 690 µs，
说明 10 ms 买的是平台调度余量，不是 `render()` 长度。
`189` 同时给 `RingBufferSampleSource` 加了**早于判空**的 `in_flight()` 计数，但没用于同步。

---

## 二、规划设计

1. **完成由 seek 控制线程判定**：`AudioPlayback::seek_to()` 等 `in_flight() == 0`，
   音频回调不写任何完成状态（对齐 Q3）。
2. **10 ms 从"固定睡眠"改成"deadline 上限"**：早归零早返回，最坏等 10 ms（对齐 Q4）。
   正常路径因此**更快**，`seek.total_us` 会下降；判据（gap ≤ 8 ms、0 丢帧）不变。
3. **超时必须带非空 reason**（G9），走既有链：
   `SeekResult.reason → audio_error → fail(handle, 11, "seek audio: …") → last_error → UI`
   （`jni_session_handle.cxx:265-290`）。超时时**不 reset 环**、先解除静音，不留下半截状态；
   上层 `resume_after_seek()` 无条件执行，会话不会卡在"线程已 join"。
4. **竞态论证（本卡成立的前提）**：
   - `in_flight_` 递增**早于** `muted_.load()`（`189` O1），覆盖"已过判空、还没读环"的窗口；
   - `set_muted(true)` 是 `release` store，回调 `acquire` load —— 此后进入的回调必被挡住，不碰环；
   - 因此 `in_flight()==0` 时没有任何回调在读环，`ring_->reset()` 安全。
5. **不引入 ticket / generation**：参考核心用 generation 防串场，是因为它有"下一曲预备"与
   多代会话；本项目 seek 由 `PlaybackSession` 保证单线程顺序调用，`in_flight()==0` 是瞬时条件，
   没有串场风险。加 ticket 是过度设计（家族树：长出新职责才分家）。
6. **不改证据 schema、不调阈值、不碰他人主线**。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/190-p16-p1b7-seek-gate-design.md` | 新增 | 本卡规划、查证、竞态论证、家族树、验收、收口 | 不复制 `183/189` 全文 |
| `tools/verify_p16_seek_gate.py` | **新增** | 钉住"固定睡眠不回潮 / 显式凭据与超时 reason 在"+ 变异自检 | 不判断 seek 数字、不改证据 |
| `backends/avsync/session/audio_playback.cxx` | 改 | 用 in_flight 凭据 + deadline 替换固定睡眠 | 不改 seek 其余四步顺序、不改 clock.rebase、不改音频流启停 |
| `.github/workflows/p2-ci-build-test.yml` | 改 | `hard-gates` 增加一步 | 不动其它 job、不加 `continue-on-error` |

新增 2 文件、改 2 个既有文件，全部落在既有族；**无新目录**。

### 判据（S1–S3）

| 判据 | 内容 |
| --- | --- |
| S1 | `audio_playback.cxx` 不再有 `sleep_for(std::chrono::milliseconds(10))` 这类固定睡眠 |
| S2 | 存在 `in_flight()` 等待 + 明确的 deadline，且超时分支写非空 `reason` 并 `set_muted(false)` |
| S3 | CI `hard-gates` 含该门禁且该段无 `continue-on-error` |

变异自检：把固定睡眠加回来、删掉超时 reason、给 CI 加回 `continue-on-error`，各自必须被抓到。

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | 六个门禁 self-test + 正式跑 | 全 PASS；CI `hard-gates` 7 步 |
| L2 | NDK clang `-fsyntax-only` + 独立 CMake 双 flavor 增量构建 | 0 错；`libkivo_player.so` 链接成功 |
| L3 | Gradle `assembleDebug` 增量 → `install -r` 到 `dd137cf7` → **带 seek 起播** | 参数 `--ei kivo.seek.ms 5000 --ei kivo.seek.at.ms 1500`（`PlayerActivity.kt:553-554`）；判据 `completed=true`、`error=""`、`seek.requests=1`、`gap ≤ 8 ms`、0 丢帧、`xrun=0` |
| L3 | 与 `189` 观测数字对照 | `seek.total_us` 应下降（不再固定睡 10 ms），且不超 G8b 150 ms |

**负向分支无法在真机伪造**（让回调卡在 `render()` 里 10 ms 需要平台配合），
所以超时 reason 由静态门禁 + 编译验证守护，真机只验"正常路径没被改坏 + seek 仍正确"。如实写明。

**设备纪律**：只操作 `dd137cf7`；`b6e950ee` 不碰；不 `uninstall`；应用私有证据保留。

---

## 五、收口（2026-11-06）

### 交付

新增 **2** 个文件（`190` 文档、`tools/verify_p16_seek_gate.py`），改 **2** 个既有文件
（`audio_playback.cxx`、CI yml）；`verify_tools_portability.py` 白名单扩到 **8** 份。
无新目录、无项目包。CI `hard-gates` 现 **7 步**、无 `continue-on-error`。

### 实现

- `AudioPlayback::seek_to()` 第 1 步：`set_muted(true)` 后等 `in_flight()==0`，
  10 ms 只作 **deadline 上限**（早归零早返回）；超时写非空 reason、解除静音、
  **不 reset 环**，走既有 `SeekResult.reason → audio_error → fail(11) → last_error → UI`。
- 不引入 ticket / generation：单线程顺序 seek，`in_flight()==0` 是瞬时条件，没有串场风险。
- 竞态依据：计数早于判空（`189` O1）+ `release`/`acquire` 配对。

### 真机三组（`dd137cf7`，同一 APK）

| 轮 | 参数 | 结果 |
| --- | --- | --- |
| A 无 seek | golden 35 s | `completed=true`，1050 帧，首帧 58.7 ms |
| B **seek→0** | `kivo.seek.ms 0` | **`total_us=23,514`（23.5 ms）、`first_frame_us=4,726`、`video_first_pts_us=0`、`pass=true`**，1050 帧 |
| C seek→5000 | `kivo.seek.ms 5000` | `landing_us=0` 但 **`video_first_pts_us=9,033,333`**；`first_frame=9.05 s`、`total=9.08 s`、`pass=false` |

### 结论

- **本卡目标达成**：静音栅栏不再固定睡 10 ms。B 轮 `total_us=23.5 ms` 落在历史
  45 轮区间（18.9–59.6 ms）内，`prepare_us` 18.8–32.8 ms 也在历史 12–36 ms 内 —— seek 主路径无回归。
- **C 轮暴露一个既有缺陷，不是本卡引入**：target=5 s 时 `landing_us=0` 与
  `video_first_pts_us=9.03 s` **自相矛盾** —— 首帧实际落在下一个关键帧，9 秒被算进
  `first_frame_us`，G8b 判红。历史 `p12f1` 的片源关键帧恰在 5 s（`landing=5000000`），
  所以从未暴露。
  本卡不碰视频 seek，且 B 轮同二进制 `pass=true`，可排除音频栅栏改动；
  定因需单独一张卡（片源关键帧间隔 × `PREVIOUS_SYNC` 语义 × `landing` 字段口径）。

### 如实说明

- 负向分支（回调卡在 `render()` 超过 10 ms）无法在真机伪造，由静态门禁 + 编译验证守护。
- **C 轮不宣称通过**，也不把 9 秒归因给本卡；这是本轮新发现的待办，不是本卡的成绩。

### 未做

不修 C 轮缺陷（另开卡）、不引入 ticket、不实现 A1 速率伺服、不改证据 schema、
不碰他人主线、不碰 `F:\kivo`、不 `uninstall`、`b6e950ee` 未碰。

---

## 六、明确不做

- 不引入 ticket / generation、不让实时回调线程写完成状态；
- 不实现 A1 速率伺服（`183` §七 Q1/Q2 仍待对方从音频渲染钟角度给判断）；
- 不改证据 schema、不调 `PacePolicy` 阈值、不碰 `av_sync_report.cxx` 等他人主线；
- 不碰 `F:\kivo`、不新建项目包。

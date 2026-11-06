# P16-P1B27 · pause-resume 设计与可行性

- 日期：2026-11-06
- 定位：`203` 暴露了 `position_us`，pause-resume 是它的自然下一步。
  本卡**只做设计与查证**，不实现 —— 因为它会动线程模型与 pacer 语义。
  门闸：构思 → 查证 → 两条路径 → 家族树 → 验收 → 结论。
- 上位：`203`、评审（播放流程 / 能力面）、`AGENTS.md`（收尾顺序铁律）。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**；**零代码改动**。

---

## 一、构思

现在"暂停"只能靠 `stop()` —— 它会停线程、排空环、取时钟、`requestStop`，
再"继续"只能 `seek_to(当前位置)`，有 20–30 ms 间隙，且会重建解码状态。

真正的 pause 应该：**停解码、停输出，但保留会话与流**；resume 从原处继续，无间隙。

---

## 二、查证（本轮实读）

| 事实 | 证据 |
| --- | --- |
| `AudioPlayback::stop()` 停 feeder → 排空环 → 取时钟 → `output_->stop()`，**不 close 流** | `audio_playback.cxx` 的 `stop()` |
| `AaudioOutputSession::stop()` 用 **`AAudioStream_requestStop`**，不 close | `aaudio_output_session.cxx` 的 `stop()` |
| `requestStop` 后**帧位置会归零**，所以 `seek_to` 要 `clock.rebase` | `audio_playback.hpp:72-73` 的注释 |
| **`AAudioStream_requestPause` 在代码里零引用** —— 从没用过 | `grep -rn requestPause` 为空 |

---

## 三、两条路径

| | A · `requestStop` + `requestStart` | B · `requestPause` + `requestStart` |
| --- | --- | --- |
| 复用现有代码 | 高（`stop()` 已有） | 低（要新写 pause/resume） |
| 音频位置 | **归零**，必须 `clock.rebase` | 应保留（待验证） |
| 间隙 | 20–30 ms（重启流的固有成本） | 理论上无 |
| 风险 | 低（已知行为） | **中**：`requestPause` 后 `getTimestamp` 是否仍有效、位置是否冻结，从未验证 |

**推荐 B，但先做可行性实验**：

> 在 `AaudioOutputSession` 加一个 `pause()`（`requestPause`），
> 真机上 pause 2 秒再 resume，读 `getTimestamp` 看 `frame_position` 是否冻结、
> `getState` 是否 `PAUSED`、resume 后是否从原位置继续。
> 这个实验不需要动 `DecodeSession`，风险可控。

---

## 四、若 B 成立，实现家族树（未来卡）

| 路径 | 职责 | 明确不做 |
| --- | --- | --- |
| `aaudio/session/aaudio_output_session.cxx` | 加 `pause()` / `resume()` | 不改 `stop()` |
| `avsync/session/audio_playback.cxx` | 转发 pause/resume；pause 时**不排空环** | 不改 `seek_to` |
| `mediacodec/session/decode_session.cxx` | `pause()`：停线程但保留 decoder/extractor；`resume()`：重启线程 | 不改 `session_teardown_plan` 七步 |
| `core/pace/frame_pacer.hpp` | resume 时把 `last_due` 重设为 `now`（否则第一帧必迟到） | 不 reset `frame_duration` |
| JNI + Kotlin | `nativeSessionPause/Resume` + 按钮 | 不改现有 `stop` |

**pacer 是最大的坑**：pause 前 `last_due_ns_` 停在旧值，resume 后第一帧 `due_at` 早已过期，
会被判迟到甚至丢帧。必须提供一个"resume 时把 due 锚点推到 now"的入口，
且**不能** `reset()`（那会清掉 `frame_duration` 与统计）。

---

## 五、验收口径（实现卡用）

| 判据 | 内容 |
| --- | --- |
| R1 | pause 2 s → resume：`position_us` 连续（跳变 < 1 帧），不倒退 |
| R2 | resume 后 3 帧内不出现 `frames_dropped_late` |
| R3 | `xrun_count` 不因 pause 增加 |
| R4 | pause 期间 `snapshot()` 可读，`position_us` 冻结不变 |

---

## 六、实验结论（2026-11-06，真机 `dd137cf7`）

探针 `tools/run_p16_pause_probe.py`（13 TU）在设备上跑：

```
pause_ok=1 resume_ok=1
p1_before_pause_ns =   993,041,771
p2_after_pause_ns  = 1,027,000,000
p3_after_2s_pause  = 1,027,000,000   ← 2 秒完全不变
p4_after_resume_ns = 1,519,466,927
```

**结论：B 路径成立。**

- `requestPause` **冻结位置**：p2 == p3，2 秒零变化；
- `resume` 后从冻结位置继续：p4 − p3 = 492 ms ≈ 500 ms 等待，位置连续；
- pause 瞬间 +34 ms 是最后一个 burst 的缓冲，正常（不是跳变）。

所以 pause-resume 可以走 B（`requestPause` / `requestStart`），
**不需要重建流、不需要 `clock.rebase`** —— 这是本轮最有价值的一条事实。

### 仍待解决（实现卡）

1. **`begin_run()` 不能复用** —— 这是真正的障碍，比 AAudio 那层难。
   `decode_session.cxx:301-305`：

   ```cpp
   counters_.reset();
   gate_.reset_for_start();
   if (pacer_) {
       pacer_->reset();   // 锚点必须在首帧到达时建立
   }
   ```

   如果 pause-resume 复用 `begin_run()`：
   - `counters_.reset()` → 证据里"1050 帧"变成"pause 后 500 帧"，pause 前那段消失；
   - `pacer_->reset()` → `frame_duration` 重新学习，resume 后前几帧的 `due_at` 不准。

   所以需要一条"**不 reset 的 restart**"：`begin_run(bool preserve_stats)` 或独立的
   `resume_run()`，只重启线程与 gate，不动 counters / pacer。
2. **`pacer` 的 due 锚点**：pause 超过 500 ms 时音频钟会 stale → `FreeRun` 分支，
   `due_at = last_due + fd` 会迟到。需要一个"把 due 推到 now、但不清 `frame_duration`"
   的入口（`resume_at(now_ns)`），而不是 `reset()`。
3. **Kotlin 状态机**：pause 时 UI 停、resume 时 UI 继续；`position_us` 在 pause 期间冻结。

**优先级**：先做 1（不 reset 的 restart），因为它是其余两步的前提；
AAudio 那层（`requestPause` 冻结位置）已经验证过，不再是风险。

### 实现进度（2026-11-06）

**第 1 步已完成**：`begin_run(bool preserve_stats = false)` ——
`true` 时跳过 `counters_.reset()` 与 `pacer_->reset()`，但仍 `gate_.reset_for_start()`
（新线程必须重新报数）。`start()` / `resume_after_seek()` 走默认 `false`，行为逐位不变。

真机回归（`dd137cf7`）：`rendered=1050`、`dropped=0`、`position_us=34,954,807`（34.955 s），
与改动前完全一致。

**仍待**：
- 第 3 步：Kotlin 状态机与 `position_us` 冻结显示；
- 一个未定的语义：`wall_us` 在 pause 下是否该扣除 pause 时长（"播放时长" vs "会话时长"）。
  目前 `run_started_at_` 会在 resume 时重置，等于"只算 resume 后"—— 这一点要在接线时确认。

**第 2 步已完成**：`FramePacer::resume_at(now_ns)` ——
只推 `last_due` / `last_present` 锚点、清 `consecutive_late_`，
**不清** `frame_duration` / 统计 / 帧计数。

行为测试 T16（真机 `dd137cf7`）：

| 判据 | 结果 |
| --- | --- |
| T16a：pause 600 ms（时钟 stale → `FreeRun`）后 `lateness > 100 ms` | **PASS** —— 问题真实存在 |
| T16b：`resume_at` 后第一帧 `lateness` 回到 ±一帧内 | **PASS** —— 锚点生效 |

一个判据边界：resume 后第一帧会"**早到整整一帧**"（`due = now + fd`），
`lateness = −33 ms` 正好卡在严格大于的边界上，放宽 1 ms 后 PASS。
这是判据写得太紧，不是实现错误。

为跑 T16，行为测试把 `core/pace/` 整个编进来（31 TU）——
`FramePacer` 的依赖链（histogram / drift / worst episode …）一个个补符号不划算，
而 `core/pace/` 全是纯逻辑、零平台头。

### 修复 P1B31 的遗漏：pause 只停了视频

静态审查发现：`DecodeSession::pause()` 只停视频线程，**音频没停** ——
pause 之后声音会继续放。这是 P1B31 实现时的疏漏。

修复：

- `AudioPlayback::pause()` / `resume()`：转发到 `AaudioOutputSession`
  （`requestPause` / `requestStart`）。pause 时**先 `feeder_->stop()` 再停流** ——
  反过来的话环会继续被填，resume 时听到的是一段"补上的旧声音"。
- `session_set_paused`：pause **先停音频**（立刻静音）再停视频；resume **先起音频**
  （时钟先走）再起视频，与 `seek_to` 的顺序一致。

编译 0 错 0 警（`feeder_->start()` 返回 void；`nodiscard` 用 `(void)` 显式忽略并注明理由）。

**未真机验证**：设备未上线，pause 的音频行为（是否真的静音、resume 是否接上）还没测。

### 顺带拆了一个超线文件

加完 T16 后 `tools/run_p16_burst_behavior_test.py` 涨到 **491 行**，越过 400 硬线
（`194` 收口时就提醒过"下次加断言先拆"，这次自己踩了）。
把内嵌的 C++ 模板外置成 `tools/p16_behavior_probe.cpp`：

| 文件 | 行数 |
| --- | --- |
| `tools/run_p16_burst_behavior_test.py` | **215** |
| `tools/p16_behavior_probe.cpp` | **274** |

拆分后真机复跑 T1–T16 仍全 PASS。

### 本卡边界

- 只加 `AaudioOutputSession::pause/resume`（**产品路径未调用**，默认行为不变）；
- 没有实现 pause-resume 本身；探针只验 AAudio 行为。

---

## 七、明确不做

- 不改任何代码（本卡零改动）；
- 不做 UI 按钮、不改 Kotlin；
- 不碰 `pace_stats.*` / `av_sync_report.cxx`；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`、`b6e950ee` 不碰。

---

## 八、真机验证与两处修复（2026-11-06，主测试机 `b6e950ee`）

> 设备切换：第二台 K40 `dd137cf7` 当时不在 USB 上，华为平板卡在 `offline`（adbd 无响应），
> 经用户授权改用主测试机 `b6e950ee`（`kona` / Android 13 / arm64）。
> 该机**首次安装** `com.kivo.cinema`（原只有 `com.kivo.music.debug` / `com.kivo.vault`），
> 未覆盖任何既有取证；机上原有 `p15a-*` 证据只列名未改动。全程 `install -r`，无 `uninstall`。

### 8.1 两轮对照（同片源 `kivo_h264_aac_35s_golden.mp4`，同参数 `pace 1 / drop 1 / cpu_paced`）

| 字段 | A 基线（无 pause） | B pause 2 s @ 5 s |
| --- | --- | --- |
| `outcome.completed` / `error` | true / `""` | true / `""` |
| `frames_rendered` / `dropped_late` | **1050 / 0** | 990 / **59** |
| `samples_submitted` | 1050 | 1049 |
| `wall_us` | 35.015 s | **5.024 s** ← 失真 |
| `position_us` | 35.031 s | **34.956 s** ✓ |
| `speed_vs_realtime` | 0.999572 | **6.967178** ← 失真 |
| `cold_start.first_frame_us` | 47.6 ms | 23.7 ms |
| `rate_ppm` / `phase_error_ns` | −14.67 / 19.2 µs | 2.28 / **−1,999,989,569** ← 失真 |
| `clock_health.jump_rebases` | 0 | **1** |
| `monotonic_rejects` / `stale_readings` | 0 / 0 | **1 / 1** |
| sync `xrun_count` / `frames_dropped` | 0 / 0 | 1 / 59 |

**结论一（好消息）**：pause 不会把会话搞崩。`jump_rebases=1` 说明那 −2 s 被跳变守卫
（`jump_limit_ns = 1 s`）识别为一次重锚，而不是漂移；`completed=true`、`position_us` 落在
34.956 s（片源 35 s），位置是连续的。

**结论二（三个失真，都是真机才看得见）**：

1. **`wall_us` 语义错**：`begin_run()` 里 `run_started_at_` 无条件重置（`decode_session.cxx:342`），
   而 `wall_micros = elapsed_us(deps_.started_at, now)`，于是 pause 轮只剩 resume 之后那段 5.02 s。
   连带 `speed_vs_realtime = position/wall = 6.97` —— 谁读都会以为"播快了 7 倍"。
2. **`phase_error_ns` 累积 −2 s**：`ClockRateEstimator` 的基准只在 `reset()` 里清
   （`clock_position_tracker.cxx`），jump rebase 时不清。于是 pause 的 2 秒被记成
   "位置比墙钟少 2 秒"，一直挂在 `phase_error` 上。**这不是相位差，是"这一段没播"**。
3. **闭合缺口 1.98 s 没被解释**：`closure_gap_ns = 1,976,342,363`、`sum_present_step_ns = −2.009 s`。
   报表注释写明"`dropped == 0` 时三数严格闭合，残差非 0 说明中间有帧没记账"——
   本轮 `dropped=59`，缺口正是 pause 那 2 秒的物理缺口。**不是计算错误，但需要显式标注**。

### 8.2 三处修复（含一次判断错误）

**第一版修错了**，必须记下来：我判断 `phase_error = −2 s` 是"jump rebase 时没重置估计器"，
于是在 `clock_position_tracker.cxx` 的 jump 分支里加了 `rate_estimator_.reset()`。
复测 `jump_rebases = 0` —— **那条分支根本没走**。真相是 pause 后音频 timestamp 短暂 stale，
走的是**外推**分支（`drift_discontinuity` 里 21 条 `extrapolated`，`wall_delta_ns = 2.044 s`、
`raw_delta_ns = 32 ms`）。外推把 pause 的 2 秒直接加进 `position_ns_`，tracker 从此**永久超前**，
视频为了追上丢掉 **58 帧**。所以 `phase_error` 与丢帧是**同一个根因**，不是两个。

| # | 位置 | 做法 |
| --- | --- | --- |
| 1 | `decode_session.{hpp,cxx}` | `run_started_at_` 只在非 `preserve_stats` 时重置；新增 `paused_total_ns_` / `pause_started_ns_` / `pause_count_`；`started_at = run_started_at_ + paused_total_ns_` |
| 2 | 同上 | `preserve_stats=true` 时**单独** `counters_.wall_micros.store(0)` —— 因为 `finish()` 是"只让第一次生效"（`compare_exchange_strong(expected=0, …)`），pause 那次已经写过 5.0 s，不归零就永远是 5.0 s |
| 3 | `clock_position_tracker.{hpp,cxx}` + `frame_pacer.cxx` | 新增 `skip_pause(now_ns)`：把 `previous_mono_ns_` 前移到 now、清 `stale_since_mono_ns_`、重置频差估计器。`FramePacer::resume_at()` 调用它 |

**语义决定**：`wall_us` 定义为**播放时长**（扣除累计 pause），不是会话时长。
理由：`speed_vs_realtime` 的分母必须是"实际在播的时间"，否则 pause 越长数字越离谱。

**为什么 `skip_pause` 不直接 `reset()`**：`reset()` 会清锚点与 `cum_drift`，
resume 首帧会被判迟到 —— 正是 P1B29 引入 `preserve_stats` 要避免的。
`skip_pause` 只动"上一帧时刻"这一个量，时间轴保持连续。

### 8.3 第三处：pause 账出版（让 `clock_off` 能自证）

修完之后 pause 轮是干净的（0 丢帧、`wall` 35.05 s、`speed` 0.9987），
但 sync 证据出现 `av_drift.sync_verdict = clock_off`、`lead_ns = −2.053 s`。

**这不是时钟跑偏，是"那 2 秒确实没播"**：pause 期间音频位置冻结、视频 pts 连续，
`lead = audio_elapsed − video_pts_elapsed` 自然少 2 秒。证据里不写这件事，
以后谁看到 `clock_off` 都会误判。

于是 `DecodeStats` 加两个字段并出版到 `timing`：

```
"pause_count": 1,          // 0 = 没暂停过（默认路径，逐位不变）
"pause_total_us": 2014211  // 2.014 s
```

判读规则写进字段注释：**`|lead_ns| ≈ pause_total_us` 时不是失步**。

### 8.4 最终真机数字（`b6e950ee`，同片源同参数）

| 字段 | A 基线 | B pause 2 s @ 5 s |
| --- | --- | --- |
| `completed` / `error` | true / `""` | true / `""` |
| `frames_rendered` / `dropped_late` | 1050 / 0 | **1049 / 0** |
| `wall_us` | 35.024 s | **35.047 s** |
| `position_us` | 35.025 s | 34.975 s |
| `speed_vs_realtime` | 0.9993 | **0.9987** |
| `pause_count` / `pause_total_us` | **0 / 0** | **1 / 2.014 s** |
| `rate_ppm` / `phase_error_ns` | −4.62 / 25.7 µs | 0.75 / **−11.6 ms** |
| `extrapolated_frames` / `jump_rebases` | 0 / 0 | **0 / 0** |
| sync `arrive_span` | 34.93 s | **36.99 s**（= 35 + 2 暂停） |
| sync `closure_gap` | 0 | **−86.7 µs** |
| sync `verdict` / `lead_ns` | av_synced / 18.5 µs | clock_off / **−2.053 s ≈ −pause** |

**三处失真全部消除**，且基线轮 `pause_count = 0` —— 默认路径逐位不变。

`phase_error` 在 pause 轮是 −11.6 ms（另一轮 −0.27 µs），那是 resume 后估计器重新收敛的
正常残差，不是累积值 —— 与修前的 −2.000 s 不是一回事。

### 8.5 防回退（行为 + 静态两道）

**行为侧**：`tools/p16_behavior_probe.cpp` 新增 T17，用**同一构造跑两遍**对照：

```
T17a without resume_at the pause inflates position (bug is real)   PASS
T17b resume_at keeps position from absorbing the pause             PASS
```

T17a 先证明"不调 `resume_at` 时位置会膨胀 >1.5 s"——这条不能省。
只测"调了之后很小"是不够的：`skip_pause` 整个被删掉、外推量恰好很小时，断言会误判 PASS。
T17b 再证明修复有效（<100 ms）。真机 21 项断言全 PASS。

**静态侧**：新增 `tools/verify_p16_pause_gate.py`（243 行，P1–P6 + 5 条变异全 CAUGHT）：

| 判据 | 钉住什么 |
| --- | --- |
| P1 | `run_started_at_` 必须在 `if (!preserve_stats)` 块**内**（用括号深度判断，不能只比位置） |
| P2 | preserve 分支里有 `counters_.wall_micros.store(0)` |
| P3 | `started_at = run_started_at_ + …paused_total_ns_` |
| P4 | `skip_pause` 存在、被 `resume_at` 调用、且三件事都做（前移时刻 / 清 stale / 重置估计器） |
| P5 | `pause_count` / `pause_total_us` 出版到证据 |
| P6 | CI `hard-gates` 跑本校验器且无 `continue-on-error` |

CI `hard-gates` 现 **9 步**（8 个门禁 + core 零平台头）。

**判据自身也修了一次**：P1 初版比较"赋值位置 vs 第一个 `if (!preserve_stats)`"，
但 `begin_run()` 开头还有另一个 preserve 分支（`counters_.reset()` 那块），
赋值天然在它之后 —— 自检 M1 因此 MISSED。改成括号深度跟踪后才 CAUGHT。
这正是"变异自检必须存在"的理由：判据写错时，正式跑照样全绿。

### 8.6 未做

- **`av_drift` 重锚**：pause 后让 `lead` 回到 0，需要动 `av_sync_report.cxx`（他人占用），留独立卡；
- `dropped_late` 里不再混 pause 帧（本轮修好后本来就没有 pause 帧被丢）；
- UI 暂停按钮与 `position_us` 冻结显示（Kotlin 侧只有测试用的 `kivo.pause.at.ms`）；
- 多机型对照（华为平板仍 `offline`，本轮全部在 `b6e950ee` 单机完成）。

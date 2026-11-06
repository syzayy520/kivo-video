# P16-P1B12 · 频差观测出版（绕开 `pace_stats.*`）

- 日期：2026-11-06
- 定位：`194` 把估计器接进了 tracker，但**没出版**——等于白跑。本卡把它送进 decode 证据，
  且**不碰 `pace_stats.hpp/.cxx`**（他人主线）。门闸：构思 → 规划设计 → 家族树 → 实现 → 验收。
- 上位：`193`（估计器）、`194`（接线）、`184`（频差账目）、`03-goals.md` G10。
- 工作目录 `C:\kivo video`；`F:\kivo` **只读**；无新目录。

---

## 一、构思

`194` 的估计器每帧在跑，但结果只挂在 `Reading` 上，没人读、不进证据。
要出版，直觉路径是 `PaceSummary`（`pace_stats.hpp`）——**但那份文件在他人手里**。

绕法：`DecodePaceSampler::publish()` 本来就在**收帧线程**被调用，而 `FramePacer`
也是同一线程写的。所以可以让 `FramePacer` 存一份 `last_rate_ppm_`，
由 `publish()` 多收两个参数一起发布 —— 同线程读，**没有数据竞争**，
也完全不经过 `PaceSummary`。

---

## 二、规划设计

1. **同线程转发**：`FramePacer::arrive()` 里抄 `reading.rate_ppm`；
   `publish()` 在收帧线程读它。`DecodePaceSampler` 的注释明说"节拍器不加锁，
   跨线程读它就是数据竞争"，这条路径满足该约束。
2. **不升 schema**：仍 `kivo.p12c.decode/19`，只在 `pace` 块里加两格。
   升版本会撞 `verify_p12_all.py` 的 `KNOWN_SCHEMAS` 枚举白名单（他人占用）。
3. **不参与判定**：`rate_ppm` / `phase_error_ns` 不进 `decide()`、不进门、不进丢帧逻辑。
4. **不碰他人主线**：`pace_stats.hpp/.cxx`、`av_sync_report.cxx` 一行不动。
5. 只操作 `dd137cf7`；`b6e950ee` 不碰。

---

## 三、自然家族树

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `docs/design/backend-core/195-p16-p1b12-rate-publish-plan.md` | 新增 | 本卡规划、家族树、判据、验收、收口 | 不复制 `194` 全文 |
| `core/pace/frame_pacer.hpp` | 改 | 存本帧频差 + 两个 getter | 不改决策逻辑、不加锁 |
| `core/pace/frame_pacer.cxx` | 改 | `arrive()` 抄值；`reset()` 清零 | 不改 `decide()`、不改 due 计算 |
| `mediacodec/session/decode_pace_stats.hpp` | 改 | 加 `rate_ppm` / `phase_error_ns` | 不改既有字段语义 |
| `mediacodec/session/decode_pace_sampler.hpp/.cxx` | 改 | `publish()` 多收两个参数并存入快照 | 不改锁范围、不改转换函数 |
| `mediacodec/session/decode_session.cxx` | 改 | 调用处传 pacer 的两个 getter | 不改其它 |
| `mediacodec/session/decode_receive_worker.cxx` | 改 | 两处 `publish()` 调用同步传参 | 不改收帧逻辑 |
| `mediacodec/session/decode_stats_renderer.cxx` | 改 | `pace` 块出版两格 | 不改其它块、不升 schema |

新增 1 文件、改 **8** 个既有文件；**无新目录、无 schema 版本变更**。

### 判据

| 判据 | 内容 |
| --- | --- |
| P1 | 真机 decode 证据的 `pace` 块出现 `rate_ppm` 与 `phase_error_ns`，且 `clock_source=audio_timestamp` 时非零 |
| P2 | `verify_p12_all.py` 仍全绿（schema 未变、无未知字段报错） |
| P3 | 帧数闭合不变：`frames_rendered` 与 `samples_submitted` 一致、`dropped=0` |

---

## 四、验收

| 层 | 命令 / 路径 | 判据 |
| --- | --- | --- |
| L1 | clang `-fsyntax-only`（改动的 3 个 TU）；独立 CMake 双 flavor | 0 错；`libkivo_player.so` 链接成功 |
| L1 | `python tools/verify_p12_all.py` | 4 层通过、20 个子校验器 exit 0 |
| L2 | `dd137cf7` 完整播放一轮 | `rate_ppm` / `phase_error_ns` 有值、1050 帧、0 丢帧、`completed=true` |
| L3 | 六个门禁 | 全 PASS；CI 7 步 |

---

## 五、收口（2026-11-06）

### 交付

新增 **1** 个文件（`195` 文档），改 **8** 个既有文件
（`frame_pacer.hpp/.cxx`、`decode_pace_stats.hpp`、`decode_pace_sampler.hpp/.cxx`、
`decode_session.cxx`、`decode_receive_worker.cxx`、`decode_stats_renderer.cxx`）。
**无新目录、无 schema 版本变更、未碰 `pace_stats.*`。**

### 实现

`FramePacer::arrive()` 抄 `reading.rate_ppm` 到 `last_rate_ppm_`（同线程）；
`DecodePaceSampler::publish()` 多收两个参数，在收帧线程里一并发布；
`DecodePaceStats` 加两格，`decode_stats_renderer.cxx` 在 `pace` 块出版。

### 验证

| 层 | 结果 |
| --- | --- |
| L1 | clang `-fsyntax-only` 0 错 0 警；双 flavor 构建 7/7 与 8/8 步、`libkivo_player.so` 链接成功 |
| L1 | `verify_p12_all.py`：**4 层全过、20 个子校验器 exit 0**（schema 未变，无未知字段报错） |
| L2 | `dd137cf7` 完整播放：`completed=true`、`error=""`、**1050 帧 / 0 丢帧**、`clock_source=audio_timestamp` |
| L2 | **`pace.rate_ppm = -0.538969`、`pace.phase_error_ns = 17149`** —— 首次出现在真机证据里 |
| L3 | 六个门禁全 PASS、CI 7 步 |

### 结论

- **A1 的测量层闭环**：估计器每帧运行，结果进了 decode 证据，且没动 `pace_stats.*`。
- 这轮数字（-0.54 ppm、35 秒累积 17 µs）说明**这台设备当前状态钟几乎完美同步**，
  与 `192` 的 8 轮压测 0 命中一致 —— 不是"问题不存在"，是"这轮硬件没抖"。
- `rate_ppm` **不参与任何判定**：丢帧、追帧、漂移门都不看它，行为与接线前逐位一致。

### 未做

不升 schema、不实现速率伺服、不改 `PacePolicy` / 漂移门、
不碰 `pace_stats.*` / `av_sync_report.cxx`、不碰 `F:\kivo`、不 `uninstall`。

### 下一步

要换速率伺服，现在有数据了：拿 `rate_ppm` 与 `drift_rebases` / 丢帧做 A/B 即可。
但**控制律是否该换仍等 `183` §七 Q1/Q2** —— 本卡只交测量，不交判断。

---

## 六、明确不做

- 不升 schema、不改 `PacePolicy`、不改漂移门、不改 `frame_pacer` 的决策；
- 不碰 `pace_stats.*` / `av_sync_report.cxx`；
- 不实现速率伺服（等 `183` §七 Q1/Q2）；
- 不碰 `F:\kivo`、不新建项目包、不 `uninstall`。

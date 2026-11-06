# P16-P1B · 音频核心评审稿：主时钟恢复与 seek 排空

- 日期：2026-11-06
- 定位：**评审用设计稿，本轮不写实现代码、不改证据、不调阈值**。门闸：构思 → 规划设计 → 自然家族树（本稿）→ **外部评审** → 实现。
- 上位：`182-p16-p1a-plan.md` §八 的三条诉求；`03-goals.md`（G6/G7/G9）；`08-p12-family-tree.md`。
- 工作目录：`C:\kivo video`（既有仓库，无新目录、无项目包）。
- 外部参照：`F:\kivo` —— 用户自研音乐播放器核心，**只读参考**，不在其中写任何文件。

---

## 一、为什么要先评审再动代码

这条链路的阈值是 G6/G7 的判据本身。`pace_policy.hpp:9-13` 写得很直白："调参必须另开卡并给出对照数据 ——
否则同一份代码的两个版本会产生两套不可比的 G6 数字"。
换控制器比调阈值更大：会同时改变 `clock_ahead`、`playback_rate`、`drift_rebases`、丢帧与 xrun 的语义。
所以在拿到对照判据之前，**只出设计、不动一行实现**。

---

## 二、现状复核（本轮实读）

### A1 · 主时钟恢复

| 位置 | 事实 |
| --- | --- |
| `clock_position_tracker.cxx:98-145` | 三种推进分支：stale 外推按墙钟、单次跳变 >`jump_limit_ns` 按墙钟重锚、正常分支按原始增量 |
| `:156-168` | 领先量改为**派生**：`lead = position − (origin_position + wall_total)`（P13-AA），不再逐帧累加 |
| `:21,26,33-61` | 台阶分类：`kStepDriftNs=30ms`、启动盲区 `1s`、一帧只认一类不连续 |
| `:192-241` | 门：`cum > 45ms` 进 `correcting_`，一次复位到 `45−5=40ms`，`drift_rebases++`；带迟滞带 |
| `pace_policy.hpp:35,45,56,63,74` | `jump=1s` / `limit=45ms` / `band=5ms` / `drift_gate_enabled` 对照开关 / `step_repay` 探针 |

**定性**：这是**带迟滞的 bang-bang 位置复位，只修相位、不修速率**。只要音频钟持续比墙钟快 1.5%，
误差就会重新攒起来、门周期性触发 —— `pace_policy.hpp:39-44` 自己记着"8 秒累积 118.79 ms、`jump_rebases` 全 0"，
`:49-51` 记着 P13-AB 注入轮 `drift_rebases=5/6/10`、总修正仅 35.2 ms、`band` 就是为"贴着阈值反复微调"加的。
它不是坏实现，是**控制律形状**与"持续频差"不匹配。

> 数值口径：本轮未逐份重算 811 份真机证据，上述数字引自源码注释与 TASKS.md 卡片。
> **实现卡必须先用既有 `tools/verify_*` 重算这些数字**，否则不许宣称复现。

**P16-P1B1 已补做（2026-11-06，见 `184-p16-p1b1-rate-events-plan.md`）**：
`tools/verify_p16_rate_events.py` 从 **162** 份含 `timeline.playback_rate` 的 P13 证据重算——
事件轮 40 份（注入 17 / 自然 23），**自然事件轮里 `drift_rebases=0`（门没响）共 6 份**并逐份列出
（`p13s-4k60-r1` rate 1.032032、`p13u-8x-r3` 1.011632、`p13v-8x-r2` 1.021176、
`p13x-8x-r2` 1.014671、`p13z-8x-r1` 1.005868、`p13z-8x-r4` 1.009277）。
另有 **90** 份早期证据不出 `drift_rebases`，按 P13-E 口径单列 `not_measured`，不冒充门沉默。

字段名更正：`clock_ahead` **不是证据里的键**，等价量在 `av_drift.trend.drift_accumulated_ns`
/ `timeline.lead_correction_ns`；`playback_rate` 在 `timeline.*`，`drift_rebases` 在 `pace.clock_health.*`。
本稿转述的 "5/6/10" 补正为 P13-AB 的 **2/5/6/10**；且 P13-AC/AC2 高达 18/78/81、AC3 才收敛到 1 —
这条演进链本身就是"bang-bang 反复复位"的直接证据。

### A2 · seek 排空

| 位置 | 事实 |
| --- | --- |
| `audio_playback.cxx:210-212` | `ring_source_->set_muted(true)` 后 `sleep_for(10ms)`，注释依据："K40 burst 192 帧 ≈ 4 ms，取 10 ms 留余量" |
| `:215-220` | `feeder_->stop()` → `seek_to` / `decoder_.flush()` → `ring_->reset()` |
| `:252-254` | `stop()` 还用 1 ms 轮询等环排空 |
| `spsc_audio_ring_buffer.hpp:21,74-75` | `reset()` 契约："只能在两个线程都停止时调用" |

所以那 10 ms 是在**猜**消费者回调已经退出，不是拿到了凭据。它换来了 F2A 的 14 轮全绿，
但把正确性挂在一台设备的 burst 时长上。

### A3 · 输出协商（**更正 `182` §八 的说法**）

`182` §八 A3 写「`aaudio_clock_source.cxx:8` 把 `frames_per_burst=192` 写死了」—— **这句不准确，本稿更正**：

- `aaudio_stream_opener.cxx:72` 用 `AAudioStream_getFramesPerBurst(stream)` 取真值；
- `aaudio_output_session.cxx:63` 把它传给 `AaudioClockSource`；
- `aaudio_clock_source.cxx:25` 的 `frames_per_burst > 0 ? frames_per_burst : kDefaultFramesPerBurst` 说明 **192 只是"传 0 时"的兜底**；
- `:138` 采样率变化时还会重算 `stale_after_ns_`。

真正的问题因此收窄为三条：① 兜底 192 **没有非空 reason**，一旦命中就是静默降级（违反 G9），
应出版 `indeterminate` + reason 而不是悄悄采用 K40 的值；② 什么条件下会传 0，未验；
③ xrun 归因、音频焦点、USB/蓝牙输出切换、第二个 SoC 矩阵仍缺，AudioUnit / WASAPI 只有接口位。

---

## 三、外部对照：`F:\kivo`（只读）

| 对照点 | 对方做法 | 路径 |
| --- | --- | --- |
| 排空凭据 | `begin_drain(session_generation)` 校验 `Playing` + generation 匹配，递增 `drain_generation` 后发 **ticket**；`complete_drain(ticket, Succeeded/Failed)` 再校验 state / 两个 generation，不匹配直接拒绝 | `engine/src/core/playback/session/drain/playback_session_drain.cpp:7-50` |
| 排空失败分类 | `renderer_.drain()` 返回值判 `Stopped`；`Timeout` → `drain_timeouts_`，其余 `BoundaryFailure` / `RenderFailure::Timeout`，并计入 `failed_drains_` | `engine/src/core/playback/runtime/drain/playback_runtime_drain.cpp:20-40` |
| 等待而非轮询 | `wait_until_writable(timeout)` 用事件句柄 + `WAIT_OBJECT_0 / WAIT_TIMEOUT`，返回 `Ready / Timeout / DeviceLost / Failed / Closed` | `engine/src/platform/windows/wasapi/renderer/wasapi_renderer_query.cpp:32-54` |
| 换流约束 | "**必须复用同一条 AAudio 流**，两路流各自 start 的交界必然有间隙或叠音" | `docs/00-plan/M1-GAPLESS-ASSESSMENT.md:45` |
| gapless 决策 | 明确记为已知缺口、写死重做触发条件、不冒充达成 | 同文档 `:48-62` |
| **A1 时钟恢复** | 对 `engine/src` 检索 `drift/phase/servo/clock_rate/playback_rate/ppm` **无命中** | 本轮 Grep |

**结论**：A2/A3 有可直接对照的成熟做法（generation/ticket drain + 事件等待 + 单流复用）；
A1 是**视频播放器特有**的问题 —— 音乐播放器没有"视频跟音频钟"这条耦合，对方核心里自然没有现成解。
所以 A1 不硬套，只能请对方从"音频渲染时钟与墙钟频差"的角度给判断。

---

## 四、候选方案

### A1 时钟恢复

| 方案 | 做法 | 代价 / 风险 |
| --- | --- | --- |
| A 继续修判据 | 保持 bang-bang，再调 limit/band/台阶规则 | **不推荐**：P13-S→Z 已证这条路会不断冒出新形状；历史证据不可比 |
| B 速率伺服 | 低通估计 `r = Δposition/Δwall`，把 `r` 用于 `due_at` 或微速率调整，让相位误差渐近归零而不是"攒够 45 ms 砍一刀" | 要定估计窗口、负载尖峰剔除、与 stale/jump 分支的关系；新增证据字段；G6/G7 需重新基线化 |
| C 只加观测 | 先出版 `rate_estimate_ppm` 与相位误差，**行为逐位不变** | 零风险，但不能直接解决缺陷 |

**建议：C → B 两阶段**。先做 C，用 8 路忙循环复现事件（历史复现率约 1/3，需 ≥8 轮），
拿到 `r` 的真实分布再决定是否替换门。理由：铁律「没有证据路径不许说完成」，换了控制器却复现不出事件等于白换。

### A2 seek 排空

| 方案 | 评价 |
| --- | --- |
| A 保留 `sleep_for(10ms)` | 否决：正确性挂在一台设备的 burst 时长 |
| B 显式 in-flight 计数 | 回调进入/退出各 `atomic++`，栅栏等到 0，带超时 + 非空 reason |
| C generation/ticket | 照 `F:\kivo`：seek 前 `begin_drain(gen)` 拿 ticket，在途完成后 `complete_drain`；ticket 不匹配拒绝 `reset()` |

**建议 B+C 组合**：generation 防串场（旧 seek 的完成不能放行新 seek），in-flight 保证 `reset()` 时点；
超时必须写非空 reason 到既有 `DecodeFailureSlot` / `last_error` 通道，不新造错误体系（G9）。

---

## 五、自然家族树（未来实现时；本轮不落代码）

| 路径 | 状态 | 唯一职责 | 明确不做 |
| --- | --- | --- | --- |
| `core/audio/clock/clock_rate_estimator.{hpp,cxx}` | 新增 | 纯逻辑：低通估计钟频比与相位误差 | 零平台头、不碰 `AAudio`、不改任何现有判定 |
| `core/audio/clock/audio_drain_ticket.hpp` | 新增 | generation + ticket 值类型与校验 | 不持有环、不持有流 |
| `backends/aaudio/session/aaudio_render_drain.cxx` | 新增 | 在途回调计数 + 显式排空等待 | 不做 seek、不碰解码 |
| `audio_playback.cxx` / `clock_position_tracker.cxx` / `pace_policy.hpp` | 改 | 接线与新增只读观测字段 | 不改既有 schema 字段语义 |
| `tools/verify_p16_audio_clock.py` | 新增 | 本卡判据校验 + 变异自检 | 不替代 `verify_p14a_metrics.py` |
| `.github/workflows/p2-ci-build-test.yml` | 改 | `hard-gates` 增加一步 | 不动其它 job |

全部落在既有族，**无新目录、无新体系**。

---

## 六、验收口径（实现卡）

- 真机 `dd137cf7`；`b6e950ee` 不碰；不 `uninstall`。
- A/B 必须**同二进制**交替；8 路忙循环，开跑前后实测空闲率；≥8 轮以覆盖约 1/3 的事件复现率。
- A1 判据：`clock_ahead` 分布、`playback_rate` 偏离 1.000、`drift_rebases`、丢帧、`xrun`、
  `rate_estimate_ppm` 的收敛时间与稳态残差；C 阶段先只验证"观测量与第三方 `playback_rate` 对得上"。
- A2 判据：seek 后 gap ≤ 8 ms、0 丢帧、静音期单列计数；**超时分支必须能被故障注入打红**。
- 新增证据字段一律挂口径前缀（沿用 P13-E 的做法），不复用裸 `p99`。

---

## 七、待用户裁决的开放问题（可直接批注）

1. **A1**：音乐播放器核心里，音频渲染时钟与墙钟的**频差**你们怎么处理？靠重采样微调，还是只做相位钳制？为什么？
2. **A1**：若用伺服，估计窗口取多长（相对 burst 4 ms / 缓冲 32 ms）？负载造成的单次大偏差要不要剔除？
3. **A1**：我们的漂移门是不是**根本不该存在** —— 视频直接跟音频钟、不做任何漂移补偿，只在设备切换时重锚？
4. **A2**：ticket 的 `complete` 由谁发？AAudio 回调线程自己发会碰实时线程，控制线程等 in-flight 归零又多一次同步，你们选哪边？
5. **A2 / A3**：drain 超时是固定值还是按设备周期换算？设备周期/采样率切换时是否重建流？
   我们 P12-F2A 的结论是"不重建流"，与你 `M1-GAPLESS-ASSESSMENT.md:45` 一致，想确认边界条件。
6. **A3**：`frames_per_burst` 取不到（传 0）时你们是拒绝启动并给 reason，还是用设备无关兜底？

---

## 八、明确不做（本卡边界）

- 不写实现代码、不改 `PacePolicy` 任何阈值、不动证据 schema；
- 不碰他人主线（`backend/CMakeLists.txt`、`CURRENT.md`、`TASKS.md`、`docs/evidence/*`、`verify_p12_*`）；
- **不在 `F:\kivo` 里写任何文件**，它只是对照资料；
- 不开能力面（PlayerState / 网络流 / 字幕 / DRM / tone mapping）；
- 评审结论未回来之前，不进入实现门闸。

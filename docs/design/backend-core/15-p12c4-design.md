# P12-C4 · 音频主时钟（设计与家族树）

- 日期：2026-10-29
- 状态：设计定稿，进入实现
- 上位：`07-p12-plan.md`（G6/G7 红线）、`08-p12-family-tree.md`（文件归位）、`TASKS.md` C4 卡
- 前序：`12-p12c-status.md`（C1 启动开销实测）、`13-p12c2-status.md`（C2 缓冲）、`14-p12c3-status.md`（C3 解码）

---

## 一、一句话结论

**音频时钟不能用墙钟算，必须用"已呈现帧位置"驱动。**
一旦驱动量是 `framePosition`，C1 那个 24–146ms 的启动开销会**自动消失**，
不需要任何常数补偿，也不需要专门去"找零点"。

---

## 二、构思：为什么不能用墙钟

G7 要求 `|video_pts - audio_clock| p99 < 40ms`。
`video_pts` 是**媒体时间轴**上的位置（从 0 开始），所以 `audio_clock` 必须落在同一个坐标系里。

最直觉的写法（也是 C1 阶段的做法）：

```
audio_clock(t) = now_mono - request_start_mono
```

它错在哪：`AAudioStream_requestStart` 返回后，**设备还要 24–146ms 才真正出声**
（C1 五次实测：146 / 25 / 24 / 146 / 157 ms，冷启动最慢）。
这段时间里墙钟一直在走，而声音还没出来 —— 于是：

| 时刻 | 真实情况 | 墙钟法的 audio_clock | 后果 |
| --- | --- | --- | --- |
| start 后 0ms | 无声 | 0 | — |
| start 后 100ms | **仍然无声** | 100ms | 同步器认为音频已领先 100ms |
| start 后 146ms | 首帧刚出声 | 146ms | 视频被判定落后 → 追帧/丢帧 |

更糟的是这个误差**每次启动都不一样**（24–146ms，6 倍波动），
所以既不能减一个常数，也不能标定一次就完事。

结论：**任何以"请求时刻"为原点的时钟都不成立。** 原点必须是"设备真的播到哪了"。

---

## 三、核心推导：零点问题会自动消失

`AAudioStream_getTimestamp` 的 `framePosition` 语义是
**"已经呈现（播完）的帧数，已含硬件延迟补偿"**，不是"已写入的帧数"。

于是：

```
audio_clock(t) = framePosition(t) / sampleRate
```

推演启动全过程：

| 阶段 | 设备真实状态 | `framePosition` | `audio_clock` | 正确吗 |
| --- | --- | --- | --- | --- |
| open 后、start 前 | 没播 | 0 | 0 | ✓ |
| start 已返回，尚未出声 | 没播 | **0**（不变） | **0** | ✓ 声音没出来，时钟就不该走 |
| 首帧真正出声 | 开始播 | 从 0 开始增长 | 从 0 开始走 | ✓ 零点就是首帧 |
| 中途 underrun 补静音 | 播的是静音 | 照常增长 | 照常走 | ✓ 时间在流逝，只是没声音 |

**所以"零点取首帧实际出声"不是一个需要额外实现的动作，而是选对驱动量的必然结果。**
C4 不需要记录 `requestStart` 时刻，也不需要减任何 latency。

### 推论：一个可验证的预言

如果上面的语义理解正确，那么真机上必然观测到：

```
(首次读到 framePosition > 0 的墙钟时刻) − (requestStart 返回时刻)
  ≈ 24–146 ms，且与 C1 用墙钟测出的启动开销吻合
```

这条就是 C4 的**第一个可证伪点**，证据里落 `startup_millis` 字段。
如果不吻合（比如总是 0 或总是几百 ms），说明 `framePosition` 的语义被我理解错了，
整个设计要推翻重来 —— 而不是"调一下参数继续走"。

> **已验证（2026-10-29，七次真机）**：`startup_millis` = 25 / 25 / 25 / 25 / 127 / 146 / 147 ms，
> 与 C1 独立测的 24–146 ms 吻合；`first_ok_frame_position` = 0 或 768（0–16 ms），
> 即首帧之前设备确实没播。设计成立。详见 `16-p12c4-status.md` §三。
>
> 附带一个设计时没预料到的结论：启动开销是**双峰**（25 ms 与 127–147 ms），
> 不是连续抖动 —— 这让"禁止固定常数补偿"从"保守做法"变成了"唯一做法"。

---

## 四、时钟模型：四段流水线

```
AAudioStream_getTimestamp          ← 唯一碰平台时钟 API 的地方
        │  (frame, time_nanos, ok?)
        ▼
frame_position_extrapolator        ← 纯数学：把 (frame,time) 外推到 now
        │  (position_ns, stale?)
        ▼
monotonic_clock_guard              ← 纯逻辑：拒绝倒退、钳制跳变
        │  (position_ns, rejected?)
        ▼
AudioClockSnapshot                 ← 纯数据：C5 只吃这个
```

四段职责单一，前三段全部零平台依赖，可在桌面机器上单测到底。
平台相关只剩最上面那一格，且它薄到"调两个 API + 翻译错误码"。

### 4.1 为什么要外推

`getTimestamp` 每个 burst 才更新一次（K40 上 burst 192 帧 = 4ms）。
若直接返回原始 `framePosition`，同一个 burst 内查 10 次会拿到 10 个相同值，
`audio_clock` 呈阶梯状 —— 对 40ms 阈值不致命（4ms 量化），
但会让 C5 的 `due_at` 计算带上 4ms 的系统性抖动，白白吃掉预算。

外推公式：

```
frame_now = frame + (now_mono_ns − time_nanos) × sampleRate / 1e9
```

**`now_mono_ns` 必须用 `clock_gettime(CLOCK_MONOTONIC)` 取，不能用 `std::chrono::steady_clock`。**
标准不保证 `steady_clock` 映射到 `CLOCK_MONOTONIC`（libc++ 实践中是，
但"实践上是"不等于"契约"）。两把尺子哪怕差 1ms，外推方向就会反。

### 4.2 陈旧判定（外推的刹车）

若 `now_mono_ns − time_nanos` 超过 2 个 burst 时长，说明 timestamp **没在更新**
（流卡住、已停、或 API 抽风）。此时必须**停止外推**，返回最后已知位置并标记 stale + 计数。

否则会无中生有：时钟自己往前跑，而设备根本没动，同步器会去追一个不存在的进度。

### 4.3 跳变钳制

正常播放每两次查询之间最多推进几十毫秒。若候选值比历史最大值**超前超过 200ms**，
判定为异常，钳制到 `历史最大值 + 200ms` 并计数。

200ms 的取法：远大于任何合理的查询间隔（C5 一个视频帧 16–42ms），
又远小于任何真实的 seek 跳变（秒级）。P12-F 做 seek 时由上层重建时钟，不走这条路径。

---

## 五、回退分级（与任务卡的偏差，说明理由）

C4 卡写的是"**回退：`getFramesWritten() / sampleRate`**"。实现时改成三档，理由如下：

| 档 | 数据源 | 语义 | 与真实呈现位置的偏差 |
| --- | --- | --- | --- |
| 1（主源） | `getTimestamp` | 已呈现 | 0（含延迟补偿） |
| 2 | `getFramesRead / sampleRate` | **同样是已呈现** | 小，精度低于 timestamp |
| 3 | `getFramesWritten − latency / sampleRate` | 已写入 | 不减 latency 会**超前整个缓冲深度**（K40 上 32ms+） |
| 4 | 保持最后值 + stale | 冻结 | 不外推，不假装在走 |

问题在于：**直接用 `getFramesWritten` 会让时钟突然超前几十毫秒**，
同步器立刻判定视频落后并追帧 —— 这正好破坏 G7 要守的 40ms。

所以顺序改成先 `getFramesRead`（同语义、无需补偿），
`getFramesWritten` 降到第三档，且必须减去用最后一次成功 timestamp 估出的 latency：

```
latency_frames = frames_written@last_ok − frame_position@last_ok
```

三档各自计数（`fallback_frames_read` / `fallback_frames_written` / `stale_freezes`），
证据里能看出这次运行到底靠哪一档撑住的。**这不是"偏离卡片"，是把卡片的兜底方案排了序**；
`getFramesWritten` 仍在链路里，只是不再当第一选择。

---

## 六、家族树

### 6.1 新增目录与文件

```
backend/include_private/video/video_plane/
├── core/audio/clock/                       ← 跨平台纯逻辑（iOS/桌面共用）
│   ├── audio_clock_types.hpp               来源枚举 + 快照结构（纯数据，零依赖）
│   ├── audio_clock_source.hpp              注入接口（纯虚，C5 只依赖这一层）
│   ├── monotonic_clock_guard.hpp
│   ├── frame_position_extrapolator.hpp
│   └── system_clock_source.hpp             无音轨时的 steady 实现
└── backends/aaudio/clock/                  ← Android 平台层
    ├── aaudio_timestamp_reading.hpp        一次读取的结果 + 失败分类（纯数据）
    ├── aaudio_timestamp_reader.hpp         唯一调用 AAudio 时钟 API 处
    ├── aaudio_clock_source.hpp
    └── aaudio_clock_stats.hpp              回退/拒绝/陈旧计数

backend/src/video/video_plane/
├── core/audio/clock/
│   ├── monotonic_clock_guard.cxx
│   ├── frame_position_extrapolator.cxx
│   └── system_clock_source.cxx
└── backends/aaudio/clock/
    ├── aaudio_timestamp_reader.cxx
    └── aaudio_clock_source.cxx

backend/tests/video/audio_plane/p12c4_clock_tests/
└── audio_clock_tests.cpp
```

### 6.2 为什么守卫和外推放 `core/`，不放 `backends/aaudio/`

C4 卡把 `clock_monotonicity_guard` 写在 `backends/aaudio/clock/` 下。
实现时移到 `core/audio/clock/`，理由与 C2 把环形缓冲从 `backends/aaudio/buffer/`
移到 `core/audio/` **完全一致**：

1. iOS 的 AudioUnit 有 `mach_absolute_time` + `AudioTimeStamp`，同样需要"外推 + 单调守卫"，
   逻辑一模一样；
2. 放在 `backends/aaudio/` 下，等于宣布"这套逻辑只属于 Android"，
   iOS 要么复制一份，要么反向依赖 Android 目录 —— 两者都是架构失败（`CURRENT.md` §六 第 1 条）；
3. 放 `core/` 才能让这三段在**桌面机器上单测到底**，这正是"三种异常"能被覆盖的前提。

`backends/aaudio/clock/` 只留真正绑 AAudio 的部分：读时间戳、翻译错误码、算 latency。

### 6.3 命名与依赖方向

```
core/audio/clock/audio_clock_types.hpp        ← 被所有人 include，自己谁也不 include
core/audio/clock/audio_clock_source.hpp       ← 只 include types
core/audio/clock/monotonic_clock_guard.hpp    ← 只 include types
core/audio/clock/frame_position_extrapolator.hpp ← 只 include types
backends/aaudio/clock/aaudio_clock_source.hpp ← include core 的 types/guard/extrapolator
```

依赖严格单向：`backends/aaudio/clock` → `core/audio/clock` → 标准库。
**`core/` 里不许出现 `<aaudio/*>`，一个都不许。**

---

## 七、单测口径（三类异常如何落地）

三类异常全部落在纯逻辑层，桌面可复现，不需要设备：

| 异常 | 注入方式 | 期望行为 | 断言点 |
| --- | --- | --- | --- |
| **时间戳倒退** | 喂 `guard(1000ms)` → `guard(900ms)` | 拒绝，返回 1000ms | `rejected == 1`，返回值不回退 |
| **时间戳跳变** | 喂 `guard(0)` → `guard(5000ms)` | 钳制到 200ms，计数 | 返回值 == 200ms，`clamped == 1` |
| **切换采样率** | 48000Hz 下走到 1000ms 位置，切 44100Hz | 位置**连续**，斜率改变 | 切换瞬间位置差 < 1ms |

补充用例（同属 C4 验收）：

| 用例 | 期望 |
| --- | --- |
| 外推正确性 | (frame=48000, t=1s, now=1.001s, rate=48k) → 1001ms |
| 陈旧不外推 | now − t 超过 2 burst → 位置冻结，`stale == 1` |
| system 时钟 | 单调不减；`source == kSystem` |
| latency 估算 | `written − position` 在首次成功后即固定，不随运行漂移 |

> 采样率切换那条最容易写错：位置 = `frame × 1e9 / rate`，rate 一变，**同一个 frame 会映射到不同时间**。
> 48000 帧在 48k 下是 1000ms，切到 44.1k 就变成 1088ms —— 直接跳 88ms。
> 正确做法是切换时记下 `(base_frame, base_position)`，之后只按增量算。

---

## 八、证据字段（`kivo_audio_file_probe` 的 JSON）

新增 `clock` 段：

| 字段 | 含义 |
| --- | --- |
| `source` | `audio_timestamp` / `audio_frames_read` / `audio_frames_written` / `system`（**必须真实**） |
| `startup_millis` | 首次读到 `framePosition > 0` 的墙钟时刻 − requestStart，用来和 C1 的 24–146ms 对照 |
| `first_frame_position` | 首次成功 timestamp 的帧位置 |
| `timestamp_ok` / `timestamp_fail` | 主源成功/失败次数 |
| `fallback_frames_read` / `fallback_frames_written` | 各回退档被用了几次 |
| `stale_freezes` | 因陈旧而冻结外推的次数 |
| `monotonic_rejects` / `jumps_clamped` | 守卫拦下的倒退/跳变次数 |
| `clock_start_ns` / `clock_end_ns` | 起止时的音频时钟 |
| `clock_span_ns` vs `frames_rendered/rate` | 两者应接近（差 ≤ 1 burst），这是"时钟跟着设备走"的直接证据 |

---

## 九、同时清掉 C3 的两笔账

### 9.1 `frames_silenced` 归因

C3 留下：`frames_silenced` 实测 128 / 320 / 198 帧，随运行波动、不随时长增长，
确认发生在"起流或收尾瞬间"，但**分不清头还是尾**。

做法：给 `AudioOutputStats` 加 `first_audible_frame`
（首次 `produced > 0` 时的 `frames_rendered` 值），于是：

```
头部静音 = first_audible_frame            （若从未拿到真实数据则为 -1）
尾部静音 = frames_silenced − first_audible_frame
```

一次就能判死，不需要再猜。这一条不做，G7 的 40ms 阈值就没有干净基线。

### 9.2 零点

见 §三 —— 不需要额外实现，是选对驱动量的必然结果，只需在证据里落 `startup_millis` 验证。

---

## 十、明确不做

| 不做 | 为什么 |
| --- | --- |
| 音视频同步决策（追帧/丢帧） | 属 C5 呈现节拍器，C4 只提供时钟 |
| seek 后的时钟重建 | 属 P12-F，C4 的跳变钳制只是防御，不是 seek 方案 |
| 重采样 | 项目红线（Direct Play First） |
| 用 `steady_clock` 做外推 | 见 §4.1，尺子必须与 timestamp 同源 |
| 在 `core/` 里 include 任何平台头 | `CURRENT.md` §六 第 1 条 |

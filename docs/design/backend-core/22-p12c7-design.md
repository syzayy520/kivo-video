# P12-C7 · 精确呈现时刻 · 设计

- 版本：V1.0
- 日期：2026-10-29
- 上游：`21-p12c6b-present-backpressure.md` §六（改写后的验收口径）、`17-p12c5-design.md`（节拍器）
- 下游：C8（A/V 合并取证）

---

## 一句话

把「CPU 睡到 `due_at` 再交帧」换成「不睡，直接 `releaseOutputBufferAtTime(due_at)`」，
让排程从**线程调度**手里转到**合成器**手里。
验收不看 fps，看**负载下提交提前量的方差**。

---

## 一、立项依据：C5 在负载下会失准

C6B 说「C5 的 `p99=0.125 ms` 是空载数字，不能当基线」。这一轮把它量化了。

**构造负载**：5 个忙循环（`sh -c 'while true; do :; done'`），实测 25% idle（正常 97%）。
**同片源**：`h264_1080p8.mp4`，30 fps 节拍，merged，各 3 轮。

| 场景 | `lateness` p50 | p95 | **p99** | **max** | 丢帧 |
| --- | --- | --- | --- | --- | --- |
| 空载（C6 基线） | 0.125 ms | 0.125 ms | **0.125 ms** | 0.26 ms | 0 |
| 负载 r1 | 0.125 ms | 0.125 ms | **1.13 ms** | 1.44 ms | 0 |
| 负载 r2 | 0.125 ms | 0.125 ms | **2.38 ms** | 3.07 ms | 0 |
| 负载 r3 | 0.125 ms | 0.125 ms | **6.38 ms** | **36.16 ms** | 0 |

**p50/p95 一动没动，p99 恶化 9–51 倍，max 恶化到 139 倍（36 ms）。**

这就是 C7 的靶子：**节拍的"平均表现"骗人，尾部才是问题。**
36 ms 的迟到意味着这一帧已经越过 2 个 vsync（60 Hz 下 16.67 ms/个），
它实际显示的时刻比 `due_at` 晚了一个到两个刷新周期 —— 而 `lateness` 完全看不见这件事，
因为它只记录"CPU 什么时候交出去"。

> **但要诚实**：这 3 轮**全部 0 丢帧**（`frames_dropped_late=0`）。
> 迟到 36 ms 的那一帧仍被判定为"在预算内"（`over_budget_frames` 只有 r3 的 1 帧）。
> 所以 C7 的收益**不是"救回丢帧"，是"让尾部迟到不再影响显示时刻"**。
> 如果验收写成"减少丢帧"，会得到一个假的"无改善"。

---

## 二、上屏时刻测不到（已试过两条路，都堵）

C6B §六 要求「先建立上屏时刻的测量」。这一节记录尝试结果，**避免下一个人重走**。

| 路径 | 结果 |
| --- | --- |
| `dumpsys SurfaceFlinger --latency <layer>` | 只返回刷新周期（`16666666`），**没有任何数据行**。三个相关 layer（`SurfaceView`、`SurfaceView(BLAST)`、`Background for SurfaceView`）全试过，HyperOS 上这个通道对 MediaCodec 直出的 SurfaceView 不产出历史 |
| `dumpsys SurfaceFlinger --frametimeline <layer>` | 输出为空 |
| NDK `ANativeWindow_getFrameTimestamps` | **不存在**。NDK 30.0.16248370 的 `android/native_window.h` 里没有这个符号，只有 `setBuffersGeometry` / `setFrameRate` / `tryAllocateBuffers` 等 |
| `ASurfaceTransactionStats_getPresentFenceFd` | 只适用于 `ASurfaceControl` 事务路径，**够不到 MediaCodec 直出** |

**结论：在「纯 NDK + MediaCodec 直出 Surface」这条架构下，拿不到逐帧的 `presentedTimeNanos`。**

不需要为此改架构（为了测一个指标去换渲染路径，是把手段当目的）。

---

## 三、改用可测的替代指标

既然测不到"什么时候上屏"，就测**"提交得有多稳"**——这是我们可以完全控制的量，
也是唯一能影响上屏时刻的输入。

### 主指标：`schedule_lead_ns`（提交提前量）

```
schedule_lead_ns = due_at_ns − release_call_time_ns
```

- `kCpuPaced`（C5）：睡到 `due_at` 再交 → 提前量 ≈ 0，尾部为负（迟到）
- `kSurfaceAtTime`（C7）：提前提交 → 提前量 ≈ 固定正值

**判据不是"提前量更大"，而是"提前量的方差更小"。**
提前量稳定 = 每帧都在同一个相位交给合成器 = 上屏时刻稳定。

尾部用 `p1`（最负的那一端）而不是 `p99`，因为风险来自"提前量不足"。

### 护栏（必须全部成立，任一破裂即失败）

| 项 | 要求 |
| --- | --- |
| `frames_dropped_late` | **0** |
| `frames_released == frames_received` | 必须 |
| `frames_rendered + 1 == frames_received` | 必须（EOS 空帧） |
| `over_budget_frames` | **0** |
| `completed` / `error` | `true` / `""` |

### 对照条件

- 同片源、同二进制、**同负载**（5 个忙循环，25% idle）
- 交替跑 `kCpuPaced` / `kSurfaceAtTime`，各 3 轮
- 负载必须先起、后跑、跑完即撤（否则轮次间漂移）

---

## 四、实现

### 4.1 接口

`MediaCodecVideoDecoder` 增加一个重载：

```cpp
/// 交出一张帧，并要求它在 `present_ns` 时刻显示（单调时钟，与 System.nanoTime 同源）。
/// `present_ns <= 0` 等价于 `release_frame(index, true)`。
[[nodiscard]] DecodeOutcome release_frame_at(std::uint64_t index, std::int64_t present_ns);
```

实现直接调 `AMediaCodec_releaseOutputBufferAtTime(codec, idx, present_ns)`。
时钟基准已验证一致：`monotonic_now_ns()` 是 `steady_clock`（libc++ 上即 `CLOCK_MONOTONIC`），
与 Java `System.nanoTime()` 同源，`due_at_ns` 可以原样传。

### 4.2 节拍器：加一个「不睡」开关

`FramePacer::Setup` 增：

```cpp
/// false = 只判定不睡（C7 的 AtTime 路径）：`arrive()` 仍算 `due_at`，
/// 但跳过 `wait_until`，`decision.wait_ns` 恒为 0。
/// 默认 true —— C5 的行为一个字都不变。
bool sleep_before_present = true;
```

`arrive()` 里唯一的改动：`kWait` 分支把 `sleep_until(...)` 包进 `if (sleep_before_present_)`，
并且 `decision.wait_ns` 相应置 0。

**为什么不做成新 action**：`PaceAction` 是给"该不该呈现"用的，睡不睡是实现策略，
两者混在一起会让 C5 的状态机多一个分支，风险大于收益。

### 4.3 会话层：模式开关

`PipelineSetup` 增（与 C6 的 `mode` 并列，互不干扰）：

```cpp
enum class PresentSchedule : std::int32_t {
    kCpuPaced = 0,       ///< C5：睡到 due_at 再交（默认）
    kSurfaceAtTime = 1,  ///< C7：不睡，交时间戳给合成器
};
```

`decode_receive_worker` 的呈现段：

```cpp
if (present_schedule == kSurfaceAtTime) {
    decision = pacer->arrive(pts);              // 不睡
    render = 判定;
    if (render) {
        // 提前量上限：最多超前 kMaxLeadFrames 帧
        const std::int64_t lead = decision.due_at_ns - now_ns();
        const std::int64_t cap  = kMaxLeadFrames * decision.frame_duration_ns;
        if (lead > cap) wait_until_ns(decision.due_at_ns - cap);
        released = decoder->release_frame_at(frame.index, decision.due_at_ns);
    } else {
        released = decoder->release_frame(frame.index, false);
    }
} else {
    /* C5 原路径，不动 */
}
```

**为什么需要提前量上限**：Surface 的 BufferQueue 只有 3–4 个槽。若一路提前提交，
槽位填满后 `release` 会阻塞，等于退化成 C5 且多了一层延迟。
`kMaxLeadFrames = 3` 是保守值 —— 正好填满队列而不溢出。

**这个 cap 不是妥协，是物理约束**：AtTime 买到的是"提交时刻由我们定"，
不是"无限提前"。

### 4.4 开关怎么进 APK

沿用 C6 的通道，新增一个 extra：

```
--es kivo.present at_time     # kSurfaceAtTime
（缺省 = cpu_paced）
```

不用 `--ei`，因为将来可能有第三种；字符串 extra 更好扩展。

---

## 五、验收

### 必须给出的两张表

1. **负载下双模式对照**（3 轮 × 2 模式）：`schedule_lead` 的 p1/p50/p99/max + 护栏五项
2. **空载下双模式对照**（同上）：证明"没有负载时两者应当无差别"——若空载就有差别，
   说明实现引入了额外延迟，要查

### 结论只有三种，必须选一个写清楚

| 结论 | 触发条件 | 后续动作 |
| --- | --- | --- |
| **有效** | 负载下 `schedule_lead` 的 p1 明显更靠近 0、护栏全绿 | 保留为默认 |
| **无差别** | 两模式指标在噪声内 | 如实记录，`kCpuPaced` 保持默认，AtTime 留作可选 |
| **更差** | 出现新的丢帧 / 提前量失控 | 回退，记录原因 |

**不接受"看起来更平滑"这种话。** 每一条结论都要有 p1/p99 数字撑着。

---

## 六、已知限制（先写在前面，别等结题才承认）

1. **`schedule_lead` 是代理指标，不是上屏时刻。** 它能证明"提交更稳"，
   不能证明"上屏更稳"。后者要 present fence，见 §二。
2. **负载是合成的**（忙循环），不等于真实场景（后台应用、温控降频、GC）。它只能证明方向。
3. **`kMaxLeadFrames = 3` 是拍的**，没有实验依据。如果实测发现 2 更稳或 4 更好，
   应当在结题时改掉这个数并记录依据。
4. **只测 1080p。** 4K 的瓶颈在解码器（7.6 ms/帧），AtTime 改不动它，
   跑 4K 只会得到"无差别"，浪费机时。

---

## 七、与 C8 的接口

C7 结题后，C8 的证据 JSON 需要多两个字段：

- `present_schedule`：`"cpu_paced"` | `"surface_at_time"`
- `schedule_lead{p1,p50,p99,max}`

C8 的 `present_jitter{p50,p99}` 保留 C5 的语义（交出时刻抖动），不要混用
——两者测的不是一回事，混在一起会让 C8 的结论没法解释。

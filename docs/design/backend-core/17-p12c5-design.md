# P12-C5 · 呈现节拍器 · 设计与家族树

- 版本：V1.0（设计定稿，实现前）
- 日期：2026-10-29
- 上游：`07-p12-plan.md` §三/§四/§六、`03-goals.md` G6、`15-p12c4-design.md`（时钟口径）、`16-p12c4-status.md`（实测数）
- 下游：`18-p12c5-status.md`（验收）、C6（分线程）、C7（精确呈现时刻）

---

## 一、目标与边界

**目标（只有一条）**：给定一帧的 `pts`，算出它在**单调时间轴**上的到期时刻 `due_at`，
然后按策略等待、呈现或丢弃，并把抖动如实记成直方图。

**这条卡的验收只看 G6**：

| 指标 | 阈值 | 来源 |
| --- | --- | --- |
| 丢帧率 | < 0.5% | `03-goals.md` G6 |
| `present_at - due_at` p99 | < 1 帧时长 | `TASKS.md` P12-C5 |
| present 间隔偏差 p99 | < 1 帧时长 | G6 原文"present 间隔抖动" |

**明确不做**（做了就是越界，会让 C6/C7 无处落位）：

| 不做 | 因为 |
| --- | --- |
| 解送/收帧分线程 | C6 的卡。C5 单线程，节拍器本身不碰线程 |
| `releaseOutputBufferAtTime` | C7 的卡。C5 在 CPU 侧睡到 `due_at`，抖动到不了 vsync 级 |
| A/V 偏差统计（`|video_pts - audio_clock|`） | C8 的卡。C5 只负责"按到期时刻呈现"，不负责"和音频比" |
| 音频解码/输出 | C3 已完成，C5 不碰 |
| seek 后重同步 | P12-F。C5 只留 `reset()` 接口 |

---

## 二、`due_at` 只能这样算

### 2.1 公式

```
due_at_mono_ns = now_mono_ns + (pts_ns - media_offset_ns - clock.position_ns)
```

三个量的语义：

| 量 | 单位 | 语义 | 谁给 |
| --- | --- | --- | --- |
| `now_mono_ns` | 单调 ns | 调用瞬间的墙钟 | `pace_now` |
| `clock.position_ns` | 媒体 ns | **此刻正在被听到/应该被看到**的媒体位置 | C4 的 `AudioClockSource` |
| `media_offset_ns` | 媒体 ns | 首帧 pts 与首帧时刻的时钟位置之差，**只建立一次** | C5 自己 |

### 2.2 为什么不是 `origin + pts`

写 `due_at = origin + pts`（origin 取 start 时刻）在真机上一定错，因为
**首帧从 requestStart 到真正出声的间隔是 25ms 或 127–147ms 双峰**（C4 七次实测）。
用 start 作原点，一半的启动会把头几帧判成"迟到 130ms"，直接丢掉。

而公式 2.1 里，`framePosition` 在首帧出声前恒为 0，所以 `position_ns` 也是 0，
`due_at(pts=0) = now` —— 启动开销被自动排除。这不是补偿，是**选对了驱动量**。

### 2.3 为什么还要 `media_offset_ns`

只用 `now + (pts - position)` 有个残留问题：**纯视频（无音轨）时 `SystemClockSource`
的 0 点在构造时刻**，而第一帧要等解码器 initialize + 首帧输出，实测几十到上百毫秒。
那段时间里 `position` 已经在走，`pts=0` 的帧一到就被判"迟到 100ms"。

于是引入 `media_offset`：在**第一帧**到来时记一次

```
media_offset = pts_first - clock.position_at_first
```

之后所有帧都用这个固定偏移。效果：

- 首帧：`due_at = now + (pts_first - media_offset - position) = now` → 立刻呈现，不计迟到；
- 之后：等价于"以首帧为零点的时间轴"，`position` 继续推进，公式仍然成立；
- 偏移**只建立一次**：后续卡顿不会被吸收（否则丢帧永远记不出来，G6 变成自证）。

这与 C4"零点取首帧实际出声"是同一条原则，只是 C5 的"首帧"是首帧**可呈现**。

### 2.4 用 C4 的实测数反推 C5 的预期

| 量 | 48k 立体声 | 44.1k 单声道 | 对 C5 意味着 |
| --- | --- | --- | --- |
| 管线延迟 `latency_frames` | 2736 帧 = 57ms | 6747 帧 = 153ms | 音频时钟位置比"已写入"晚 57/153ms；C5 直接用 `position`，**不需要**再减 |
| 启动开销 | 25 / 127–147 ms 双峰 | 同 | 由 §2.3 的 offset 吸收 |
| 单调回退 | 每次 2–3 次 | 每次 8 次 | `position` 已被 C4 守卫滤波，C5 不重复做，但要计 `monotonic_rejected` 供证据 |

> **注意一个方向性错误**：C4 文档里的 57ms 是"写入位置 − 呈现位置"。
> 有人会顺手把它当成"视频要提前 57ms"，那是错的 —— `position` 已经是呈现位置，
> 再减一次就把视频提前 57ms。**C5 一律不再做任何延迟补偿**，这条写进单测断言。

---

## 三、动作与阈值

沿用任务卡的四档，不新造档位：

| 条件 | 动作 | 说明 |
| --- | --- | --- |
| `now < due_at - 2ms` | `kWait` | 睡到 `due_at`，不忙等 |
| `|now - due_at| <= 2ms` | `kPresent` | 直接呈现 |
| `due_at + 100ms < now` 且未追帧 | `kDrop` | 丢弃该帧（不呈现），计 `frames_dropped` + reason |
| 连续 `> 10` 帧落后 | `kCatchUp` | 进入追帧，连续丢弃直到落后回到 1 帧内 |

2ms / 100ms / 10 帧三个数**照抄任务卡**，不调参。调参必须另开卡并留下对照数据。

### 3.1 任务卡没写、但必然踩到的三个坑

**(1) `pts` 可能不单调或重复。**
B 帧重排后解出的 pts 是单调的，但坏片源、时间基切换、首帧重复都会给出
`pts <= pts_prev`。若照公式算，`due_at` 会倒退，于是"睡 −3ms"或两帧同时呈现。

对策：`due_at = max(due_at, last_due_at + frame_duration)`。**单测 C5-6 守这条。**

**(2) 时钟位置会跳变。**
C4 已实测 `getTimestamp` 会回退（每轮 2–8 次）。若某次 `position` 一次前进 500ms
（换采样率、设备重建、seek），照公式算 `due_at` 会后退 500ms，于是整段被判"迟到"→ 全丢。

对策：`clock_position_tracker` 比较"位置增量"与"墙钟增量"，偏差超 `jump_limit`（默认 1s）
即视为跳变，**重锚** `media_offset += (pos_delta - wall_delta)`，保持 `due_at` 连续，
并计数 `jumps_rebased`。**单测 C5-7 守这条。**

**(3) 时钟可能停更（`stale`）。**
C4 的 `stale=true` 表示"本轮没能推进，用的是最后已知值"，正常只持续一个 burst（4ms）。
若 C5 直接拿陈旧值算，`pts - position` 会随墙钟增长，帧会越等越晚，最后集体判迟到。

对策三级降级（见 §四）。**单测 C5-8、C5-9 守这条。**

---

## 四、时钟健康度三级降级

| 级别 | 触发条件 | 位置怎么取 | 证据字段 |
| --- | --- | --- | --- |
| `kFresh` | `stale == false` | 设备值 | — |
| `kExtrapolated` | `stale == true` 且陈旧 < 500ms | 最后已知值 + 墙钟增量 | `stale_frames` |
| `kFreeRun` | 陈旧 ≥ 500ms | **改用自生成时间轴**：`due_at = last_due_at + frame_duration` | `free_run_frames` |

`kExtrapolated` 的合理性：音频缓冲 32ms、burst 4ms，一次 `stale` 最多丢一个 burst。
按墙钟补 4ms 与真实位置误差在微秒级，比"冻结位置"好得多。

`kFreeRun` 是**显式降级**，不是静默。它一旦发生，证据里 `clock_health` 必须写 `free_run`，
并且 `free_run_frames > 0`。理由同 C4：把"精度已降级"从证据里抹掉，G6/G7 就没有意义。

---

## 五、家族树

```
backend/
├── include_private/video/video_plane/core/pace/
│   ├── pace_action.hpp            # 动作枚举 + 判定结果（纯数据）
│   ├── pace_policy.hpp            # 阈值与判定规则声明
│   ├── clock_position_tracker.hpp  # 时钟读取 + 陈旧外推 + 跳变重锚
│   ├── pace_stats.hpp             # 零分配直方图与分位数
│   ├── frame_pacer.hpp            # 编排：pts → due_at → 动作 → 等待 → 记账
│   └── pace_now.hpp               # 单调时钟与睡眠函数（可注入）
├── src/video/video_plane/core/pace/
│   ├── pace_action.cxx            # 动作名（进证据 JSON）
│   ├── pace_policy.cxx            # decide_pace()：**纯函数**，无状态
│   ├── clock_position_tracker.cxx
│   ├── pace_stats.cxx
│   ├── frame_pacer.cxx
│   └── pace_now.cxx
└── tests/video/pace/p12c5_pace_tests/
    └── frame_pacer_tests.cpp      # C5-1 … C5-12
```

### 单文件职责（一句一文件）

| 文件 | 做 | **不做** |
| --- | --- | --- |
| `pace_action` | 定义"这一帧怎么办"的数据形状与可读名 | 不做判定 |
| `pace_policy` | 阈值 + 由 `(lateness, 连迟帧数)` 推出动作 | 不读时钟、不睡觉、不记账 |
| `clock_position_tracker` | 把 `AudioClockSource` 变成"可信位置 + 健康度" | 不算 `due_at` |
| `pace_stats` | 直方图、分位数、计数 | 不做判定、不碰时钟 |
| `frame_pacer` | 编排：建 offset、算 `due_at`、等、记账 | 不碰 MediaCodec、不认识 `ANativeWindow` |
| `pace_now` | `monotonic_now_ns()` / `wait_until_ns()` | 不含任何策略 |

### 为什么整个 `core/pace/` 一行平台类型都没有

`core/` 出现 `<android/*>` 或 `<GLES*>` 即为架构违规（`07-p12-plan.md` §六 第 6 条）。
C5 的实现只用 `std::chrono::steady_clock` + 一个 `int64` 纳秒接口，
所以**桌面 MSVC 与 arm64 NDK 编的是同一份源**，单测不需要设备。

这与 C4 的 `core/audio/clock/` 是同一条理由，也是 §六 里那条"节拍完全共用"的落地方式。

### 依赖方向

```
core/pace/  ──> core/audio/clock/audio_clock_source.hpp（只依赖接口，一个纯虚类）
     │
     └── 不依赖 backends/、不依赖任何平台头
```

`FramePacer` 收 `core::audio::AudioClockSource&`。**这不是"节拍依赖音频"**：
`SystemClockSource` 同样实现该接口，纯视频时注入的就是它，证据里 `clock_source` 会写 `system`。
新建一套平行的时钟接口才是真的错——那会有两套"媒体位置"语义，C8 算 A/V 偏差时必然对不上。

---

## 六、测量口径（这一节决定证据能不能被信）

### 6.1 `present_at` 到底是什么

`present_at` = **调用 `releaseOutputBuffer(render=true)` 之前的瞬间**。

它**不是**「像素到达眼睛的时刻」。后者要 SurfaceFlinger 的 present fence 或
`dumpsys SurfaceFlinger --latency` 才能拿到，P12 阶段不做。

所以 G6 在本阶段的口径是 **调度抖动**（我们叫它几点呈现，实际几点上交），
不是**上屏延迟**。这一点必须在状态文档里写明，否则会被读成"已证明 40ms 内上屏"。

C7 用 `releaseOutputBufferAtTime` 后，排程交给合成器，那时才能谈 vsync 级；C5 不谈。

### 6.2 两个直方图

| 直方图 | 定义 | 对应 G6 的哪半句 |
| --- | --- | --- |
| `lateness` | `present_at - due_at`（有符号，早为负） | `present_at - due_at` p99 |
| `interval_deviation` | `(present_at[i] - present_at[i-1]) - frame_duration` | "present 间隔抖动" p99 |

桶宽 0.25ms，范围 ±128ms，外加两个溢出桶（`< -128ms`、`> +128ms`），共 1026 桶 ≈ 8KB，**零分配**。

分位数从桶取（误差 ≤ 0.25ms），比保留全样本更省，也避免"为了算 p99 在热路径上 `new`"。

### 6.3 `frame_duration` 从哪来

不信容器声明的 fps（`07 §六` 已禁"按名字/声明假设"）。取相邻帧 pts 差的正值，
钳在 `[1ms, 200ms]`，取最近一次的样本作为当前帧时长。首帧用 40ms 兜底。
证据里同时写 `frame_duration_ns` 与 `derived_fps`，让第三方能核对。

---

## 七、与任务卡的偏差（逐条报备）

| 卡片原文 | 实际做法 | 理由 |
| --- | --- | --- |
| 落位 `core/pace/` | 一致 | — |
| 回退/丢弃策略四档 | 一致，另加"跳变重锚"与"三级降级" | §3.1(2)(3)：不处理会整段丢帧或越等越晚 |
| `kDrop` 只记 reason | 记 reason + `lateness_ns` | 没有迟到量级的丢帧无法归因 |
| 时钟注入 | 复用 `core::audio::AudioClockSource`，不新建接口 | §五：两套位置语义必然对不上 |
| 验收 24/30/60fps | 一致；30fps 复用 P12-B 的 1080p 源，24/60 新生成 | 原片源只有 30fps 一档 |
| 证据里 `clock_source` | 直接用 C4 的 `audio_clock_source_name()` | 同一个字段，不许两套拼写 |

### 一处需要提前说清的"不算达标"

C5 的真机取证用 **`SystemClockSource`**（纯视频，无音轨），因为 C5 不碰音频。
所以这轮证据能证明 **G6（节奏）**，**不能**证明 G7（音画同步）——
G7 要 `audio_clock` 驱动 + 音视频同时跑，那是 C8 的闸门。
把 C5 的数字写成"已达成 G7"属于红线第 5 条（没有实测数字就宣称达标）。

---

## 八、可证伪点

设计如果错了，下面任一条会先炸，而不是等到最后：

1. **首帧被判迟到**：若 `silence` 式的 `frames_dropped` 在开头就非 0，
   说明 `media_offset` 建立时机错了（应在第一帧**可呈现**时，不是 `start()` 时）。
2. **p99 接近帧时长**：若 p99 ≈ frame_duration，说明 `due_at` 系统性偏移一帧，
   `media_offset` 的符号或 `position` 的方向搞反了。
3. **`jumps_rebased` 为 0 但 `frames_dropped` 很多**：说明跳变检测阈值太松，
   把真实跳变当成了正常推进。
4. **`free_run_frames > 0`**：说明时钟停更超过 500ms，`stale` 处理不成立，要回查 C4。

任何一条成立 → 停手回查，不许"调一下参数继续走"。

---

## 九、单测口径（`p12c5_pace_tests`）

注入假时钟 + 假等待，全部确定性，不依赖真实时间：

| 编号 | 用例 | 断言 |
| --- | --- | --- |
| C5-1 | 早到 20ms | 动作 `kWait`，`wait_ns == 20ms ± 1us` |
| C5-2 | 正点（±1ms） | 动作 `kPresent`，不睡 |
| C5-3 | 迟到 50ms（< 100ms） | 动作 `kPresent`，计迟到但不丢 |
| C5-4 | 迟到 150ms（> 100ms） | 动作 `kDrop`，`frames_dropped == 1`，reason 非空 |
| C5-5 | 连续迟到 11 帧 | 第 11 帧起 `kCatchUp`，`catch_up_events == 1` |
| C5-6 | `pts` 重复 / 倒退 | `due_at` 前进一帧，不倒退，不睡负值 |
| C5-7 | 时钟一次前进 500ms | `jumps_rebased == 1`，`due_at` 连续（相邻差 ≤ 1 帧） |
| C5-8 | `stale` 持续 30ms | 走 `kExtrapolated`，不丢帧 |
| C5-9 | `stale` 持续 800ms | 降到 `kFreeRun`，`free_run_frames > 0`，仍按帧率推进 |
| C5-10 | 已知分布 100 帧 | p50/p95/p99 与手算一致（±0.5ms） |
| C5-11 | 溢出桶 | 迟到 200ms 落在 `> +128ms` 桶，`max` 仍准确 |
| C5-12 | 首帧 offset | 首帧 `due_at == now`，不判迟到；第 2 帧起按 pts 差推进 |
| C5-13 | 阈值边界 | 正好 2ms 不睡、正好 100ms 不丢；`sane()` 拒绝自相矛盾配置 |
| C5-14 | 分位数准确 | 100 个 0–99ms 样本，p50/p95/p99 误差 ≤ 0.5ms |
| C5-15 | 溢出桶保留极值 | `±128ms` 之外返回精确 `min`/`max`，不编桶中心 |
| C5-16 | 睡过头 | 0.5ms 过冲同时进"迟到"与"间隔"两个直方图 |
| C5-17 | 分位数不超过极值 | 全部样本同桶时 `p99 ≤ max`（真机首轮暴露） |

> C5-13 ~ C5-17 是设计定稿后补的：C5-13/14/15 是"把口径钉死"，
> C5-17 是**真机证据自相矛盾**（`p99 = 0.125ms` 而 `max = 0.079ms`）逼出来的。
> 补用例之前先把实现改成"分位数钳进 `[min, max]`"，不是改断言迁就实现。

**只测判断，不测硬件**：这组测试在 Windows 上跑，与 C4 的做法一致。
真机只回答"这套判断在真实时钟上跑出来的数是多少"。

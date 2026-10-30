# P12-D4 验收：收尾顺序收敛成一份可测的数据

- 版本：V1.0
- 日期：2026-10-29
- 设计：`31-p12d4-design.md`
- 判据：**`SessionHandle` 释放顺序唯一性有单测守护**
- 结论：**达成**。顺序从"三处代码的先后"变成"一份 constexpr 表 + 一个执行器"，
  单测盯住这张表，真机两轮回归证明行为未变。

---

## 一、判据为什么不是"补个测试"就完事

顺序原本藏在三个地方：

| 位置 | 表达方式 | 谁能发现它错了 |
| --- | --- | --- |
| `session_stop()` | 三条语句的先后 | 读代码的人 |
| `session_destroy()` | 四条 `reset()` 的先后 | 读代码的人 |
| `SessionHandle` 成员声明 | **声明逆序 = 析构顺序** | 基本没人 —— 它不写在任何函数里 |

而这两个函数都在 `jni_session_handle.cxx` 里，那个文件 include `<android/native_window_jni.h>`，
**桌面测试树编不进来**。所以"有单测守护"在原来的结构上是不可能的 ——
要么放弃判据，要么把顺序提成数据。选了后者。

---

## 二、收敛后的形状

`backend/include_private/video/video_plane/backends/mediacodec/session/session_teardown_plan.hpp`
（纯头文件，无 `.cxx`，无平台头）：

```
enum class SessionTeardownStep { kStopVideo, kSampleAudioPosition, kStopAudio,
                                 kResetVideo, kResetAudio, kResetWindow, kResetClock }
kSessionDestroyPlan  = 上面七步，按顺序
kSessionStopPlanSize = 3            // 停止 = 销毁计划的严格前缀
run_session_teardown(plan, hooks)   // 唯一的执行器：一个 switch，消费 plan
```

JNI 侧只提供一个 `SessionTeardownHooks`（七个函数体：每一步"做什么"），
顺序一行都不写。于是：

- **停止** `run_session_teardown(session_stop_plan(), hooks)` → 前 3 步；
- **销毁** `run_session_teardown(session_destroy_plan(), hooks)` → 全 7 步。

"前缀"这个形状是刻意的：如果停止自己再写一份 `{停视频, 取位置, 停音频}`，
两份列表可以各自演化，"唯一性"就只剩口号。

**没有新开目录**（设计稿 §四 已说明）：一个新头文件开新族必然是个空壳，
违反家族树规则第 2 条。放 `session/` 是因为这七步讲的全是
`DecodeSession` / `AudioPlayback` / `ANativeWindow` / `AudioClockSource` 谁先死 ——
正是 `session/` 已经在编排的那批。

---

## 三、单测：6 例 / 36 断言，外加两次变异

新增目标 `kivo_p12d4_lifecycle_tests`（不塞进 pace / pipeline 的测试：
改这份顺序的人多半在重构 JNI，放在别处会被当噪声跳过）。

| # | 断言 | 守的是什么 |
| --- | --- | --- |
| D4-1 | 销毁计划逐项等于预期七步 | 顺序本身 |
| D4-2 | 停止是严格前缀、且**同一块内存** | 唯一性 + 常量没和数组脱钩 |
| D4-3 | 所有 `reset` 排在所有 `stop` 之后 | "先停后放"这条硬约束 |
| D4-4 | 采样夹在"停视频"与"停音频"之间 | C8 那个覆盖 bug 的形状 |
| D4-5 | 用记账钩子跑两个计划，记录逐项等于计划 | **执行器没有自己的顺序** |
| D4-6 | 停止跑一遍 + 销毁跑一遍，采样只生效一次 | 幂等（不覆盖已取到的位置） |

**"通过"不能证明测试有牙齿，所以做了变异检查**（改坏 → 跑 → 还原）：

| 变异 | 结果 |
| --- | --- |
| ① 把 `kResetClock` 提到最前（最常见的"整理代码"错误） | **11 项断言红**（D4-1/3/5） |
| ② 采样与停音频互换（其余顺序完全正确） | **6 项断言红**（D4-1/4/5/6） |
| 还原 | 36 项全绿 |

变异 ② 值得单说：它是**唯一一处"看起来完全正常、实际会静默出错"的改动** ——
所有资源都按正确顺序释放，只有那个位置读到的是停流后的回退档。
D4-4 与 D4-6 同时咬住它，说明这两条不是凑数的。

---

## 四、真机回归：Kotlin 收敛之后行为必须逐项对得上

D2 留了一笔账（`08-p12-family-tree.md` 三之八 写了"收敛归 D4"）：
`PlaybackSession.kt` 与 `PlayerActivity` 的内联逻辑重复。这次收掉了。

`PlayerActivity` 现在只剩：生命周期回调、Surface 传递、轮询、证据落盘、extra 解析。
句柄创建/释放、两个 fd 的开与关、open/config/start/rebind/stop 全部走 `PlaybackSession`。
Activity 里**不再存 `sessionHandle` / `mediaDescriptor` / `audioDescriptor` 任何副本** ——
留着副本就又有"两处各自释放"的可能，而那正是要消掉的东西。

判据是"数字不变"，所以重跑了 C 组与 D1 各一条取证路径：

| 轮 | rate | av_drift p99 | 帧 recv/rend/drop/disc | rebuild | 结果 |
| --- | --- | --- | --- | --- | --- |
| C8 归档 r1 | 0.999984 | 125 000 ns | 181/180/0/— | — | 基线 |
| C8 归档 r2 | 0.999985 | 125 000 ns | 181/180/0/— | — | 基线 |
| **C8 回归 r1** | 0.999983 | 125 000 ns | 181/180/0/0 | 0 | 一致 |
| **C8 回归 r2** | 0.999985 | 125 000 ns | 181/180/0/0 | 0 | 一致 |
| **C8 回归 r3** | 0.999986 | 121 500 ns | 181/180/0/0 | 0 | 一致 |
| D1 归档 r2 | 0.999979 | —（system） | 181/149/0/31 | 1 | 基线 |
| **D1 回归 r1** | 0.999973 | —（system） | 181/163/0/17 | 1 | 一致 |

`clock_source` 三轮 C8 全是 `audio_timestamp`，D1 是 `system`（无音轨，正确降级）；
`xrun_count` 全 0、`timestamp_fallback_count` 全 0、`completed` 全 true、`error` 全空。
四份证据全部通过 `tools/verify_p12c_sync.py` 独立复算（帧数闭合 + 播放速率 + 分位数重放）。

D1 那轮的空窗丢帧自洽：`574 ms × 30fps = 17.2`，实际 **17**。

> C8 回归 r1 的 `max` 是 639 µs（归档是 162 µs）。单个离群点，p99 没动。
> 记在这里是因为"三次回归两次都在 0.12 ms、一次 0.64 ms"和"一致"不是同一句话 ——
> 判据（G7 = 40 ms）都远过，但这个尾巴是真的存在。

---

## 五、顺带修掉的两个真 bug

收敛过程中遇到两个"不是重构、是缺陷"的东西，一并修了：

**① `session_destroy()` 从来不取音频位置。**
之前只有 `session_stop()` 取。所以"直接销毁、没先停止"这条路径上
`audio_position_at_video_stop_ns` 恒为 −1，`kivo.p12c.sync/1` 的
`timeline.audio_position_at_video_stop_ns` 会写 −1 而没有人解释为什么。
现在销毁走完整七步，自然包含采样；同时加了幂等保护
（`>= 0` 就跳过），避免"先 stop 再 destroy"时用停流后的回退档覆盖好数据。

**② 结束证据被反复重写。**
`refreshStatus()` 在 `completed=1` 之后仍被 400 ms 定时器反复调到，
`finishPlayback()` 每次都重写一遍 `decode/6` 和 `-sync.json`。
数字没变所以一直没人发现，但它意味着**最后一次写可能落在会话已经半停的时刻**。
改成"成功才封口"：成功一次就不再写，失败则下一轮重试（不会因为一次瞬时失败丢证据）。

---

## 六、诚实清单

1. **单测守的是"计划表 + 执行器"，不是"每个钩子函数体都对"。**
   比如 `reset_window()` 里写成 `audio.reset()` 这种错，36 条断言一条都不会红。
   那一层只能靠真机回归 —— 这次跑了两条路径，但两条路径覆盖不到全部七个钩子
   （无音频轮走不到 `stop_audio` / `reset_audio` 的真实分支）。
2. **`SessionHandle` 的成员声明顺序这个"第三处"仍然靠注释维持。** 单测测不到 C++ 成员顺序，
   因为它定义在 `.cxx` 里、桌面测试看不到这个类型。缓解办法是让它成为计划的"兜底"
   （显式收尾总会先跑），但"有人按字母序重排成员"这件事本身仍然拦不住。
3. **Kotlin 收敛的等价性靠两轮回归证明，不是形式化证明。** 交互路径（不传 `kivo.evidence`、
   不自动退出）只做了冒烟，没有逐项复跑 —— 它的代码路径与取证路径共用同一个
   `finishPlayback`，但重复调用行为不同（取证模式会 `finish()`，交互模式不会）。
4. **"失败就重试"有一个新的边界**：如果证据**持续**写不进去，取证模式下不会 `finish()`，
   应用会一直停在那里等重试。这比"丢了证据悄悄退出"好，但脚本侧的等待循环会超时。
   取舍写在这里，别当成 bug 修。
5. **变异测试只证明了这两种改动会被抓住**，不能证明"任何错误改动都会被抓住"。
6. **只验了 K40 一台设备、两种 6 秒片源**。没有 4K、没有 HEVC、没有带 B 帧的片源。
7. **`session_stop` 与 `session_destroy` 的"停止幂等"依赖被调用方本身幂等**
   （`DecodeSession::stop` / `AudioPlayback::stop` 各自幂等）。这一层没有单测，
   只有 C8 与 D1 两次"先 stop 再 destroy"的真机路径。

---

## 七、对 P12-D 的意义

D4 是 D 组最后一张**功能**卡。但 **P12-D 没有因此结题**：

`TASKS.md` 里 P12-D 的主题是「Surface 重建 + 多实例 + **上屏时刻**」，前两项做完了，
**上屏时刻（present fence）一件没做**。它从 C8 起就是明确欠账：

- 没有 present fence 就拿不到"这一帧什么时候真的显示在屏幕上"；
- 于是默认 `surface_at_time` 路径的 `av_drift` 永远 `measurable: false`；
- 于是"音画同步已达标"只能限定在 `cpu_paced` 路径上说。

所以指针给 **P12-D5（上屏时刻）**，不给 P12-E。
把"上屏时刻"悄悄挪进 P12-E 或干脆不提，等于用一张卡的名义做完了另一张卡的事。

---

## 八、产物

- 设计：`31-p12d4-design.md`
- 新增：`backend/include_private/.../session/session_teardown_plan.hpp`（纯头文件）
- 新增：`backend/tests/video/session/p12d4_lifecycle_tests/session_teardown_tests.cpp` + CMake 目标
- 改：`jni_session_handle.cxx`（停止/销毁都走计划）、`PlaybackSession.kt`（唯一入口）、
  `PlayerActivity.kt`（不再持有句柄与 fd）、`MultiPlayerActivity.kt`（跟新 API）
- 证据：`docs/evidence/p12d/p12d4-{c8reg-r1..r3,d1reg-r1}{,-sync}.json`（8 份）
- 回归：`ctest -R kivo_p12` 6/6；`tools/verify_p12c_sync.py` 四份全过

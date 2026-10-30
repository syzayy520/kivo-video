# P12-D4 设计：会话收尾顺序收敛成一份可测的数据

- 版本：V1.0
- 日期：2026-10-29
- 关联：`27-p12d1-status.md`、`25-p12c8-status.md`、`08-p12-family-tree.md` §三之十
- 卡片判据：**`SessionHandle` 释放顺序唯一性有单测守护**

---

## 一、为什么这张卡不是"补个测试"

判据里"有单测守护"这四个字，在今天的代码结构上**做不到** —— 顺序写在
`android/app/src/main/cpp/jni/jni_session_handle.cxx` 里，那个文件
`#include <android/native_window_jni.h>`，桌面测试树根本编不进来。

所以真正要做的事是：**把顺序从"两段代码的执行先后"变成"一份可读的数据"**，
再让两段代码都去消费它。顺序成了数据，才有东西可测。

先数一遍现在顺序藏在几个地方：

| 位置 | 表达方式 | 谁能发现它错了 |
| --- | --- | --- |
| `session_stop()` | 三条语句的先后 | 读代码的人 |
| `session_destroy()` | 四条 `reset()` 的先后 | 读代码的人 |
| `SessionHandle` 成员声明 | **声明逆序 = 析构顺序** | 基本没人 —— 它不写在任何函数里 |

第三条是最阴的：`system_clock` 必须比 `session` 活得久，这条约束只活在一句注释里
（"**必须声明在 session 之前**"）。谁哪天按字母序重排成员，编译照样过、
单测照样绿，真机上偶发悬空指针。

这一版的判据就是：**把这三处收敛成一处，并让单测能盯住那一处。**

---

## 二、收尾顺序到底是什么，为什么是这个顺序

七步。每一步的"为什么"都对应一个已经踩过的故障：

| # | 步骤 | 不做会怎样 |
| --- | --- | --- |
| 1 | 停视频（`DecodeSession::stop()`） | 收帧线程还在往节拍器塞帧，节拍器正在被另一线程读 |
| 2 | 取"此刻"的音频位置 | 停完音频再取，读到的是停流后的回退档 —— C8 修过的那个覆盖 bug |
| 3 | 停音频（`AudioPlayback::stop()`） | 停流之前取不到时钟终值；缺这一步 `clock_source` 会退回 `system`（C8 第一轮） |
| 4 | 放会话 | 节拍器持有 `AudioClockSource*` 裸指针，会话先死 = 指针悬空 |
| 5 | 放音频 | 时钟属于 `aaudio::OutputSession`（在 `AudioPlayback` 内），音频先死 = 时钟悬空 |
| 6 | 放窗口 | 解码器（在会话内）还指着 `ANativeWindow*`，窗口先放 = D1 那个崩 |
| 7 | 放时钟 | `SystemClockSource` 被节拍器裸指针持有，最后一个放 |

**两条硬约束从这张表直接读出来**：

1. **所有 `stop` 必须早于所有 `reset`。** 任何"先放资源再停线程"的顺序都会让线程
   摸到已释放对象。
2. **所有权链必须是 `session → audio → clock`、`session → window`。**
   持有裸指针的一方先死 —— 这就是"先声明者后亡"那句注释的数据版本。

---

## 三、收敛方式：计划是数据，执行只有一个函数

新增一个头文件（纯数据 + 一个模板执行器，无 `.cxx`、无平台头）：

```cpp
enum class SessionTeardownStep { kStopVideo, kSampleAudioPosition, kStopAudio,
                                 kResetVideo, kResetAudio, kResetWindow, kResetClock };

inline constexpr std::array<SessionTeardownStep, 7> kSessionDestroyPlan{...};

// 停止 = 销毁计划的**严格前缀**，不是另一份列表。
inline constexpr std::size_t kSessionStopPlanSize = 3;

template <typename Hooks>
void run_session_teardown(std::span<const SessionTeardownStep> plan, Hooks& hooks);
```

`Hooks` 只需要七个成员函数。JNI 侧写一个把 `SessionHandle` 包起来的 `Hooks`，
测试侧写一个只记账的 `Hooks` —— **两边跑的是同一个 `switch`**。

为什么是模板而不是 `std::function` 数组：这个路径一次会话走两趟，性能无所谓；
但 `std::function` 会让"执行器"变成一个可替换对象，而这份设计要的恰恰是
"执行器只有一份，不许替换"。

### 3.1 停止为什么是前缀，不是另一个列表

如果 `stop` 自己再写一遍 `{停视频, 取位置, 停音频}`，那"唯一性"就还是假的 ——
两份列表可以各自演化。做成前缀之后：

```cpp
session_stop   → run_session_teardown(session_stop_plan(),    hooks);  // 前 3 步
session_destroy→ run_session_teardown(session_destroy_plan(), hooks);  // 全 7 步
```

单测只要断言 `stop_plan` 是 `destroy_plan` 的前缀，任何一方改动都会红。

### 3.2 无音频时那一步怎么办

**计划不变，钩子自己判空。** 不给计划加 `has_audio` 分支 —— 一旦计划随运行时状态变形，
"前缀"关系就不再是编译期常量，测试也就只能测其中一种分支。

`kSampleAudioPosition` 的钩子写成：

```cpp
if (audio == nullptr || audio->clock() == nullptr) return;   // 没音频，跳过
if (audio_position_at_video_stop_ns >= 0) return;            // 已取过，不覆盖
audio_position_at_video_stop_ns = audio->clock()->now().position_ns;
```

第二行是这次顺带修掉的**真 bug**：`session_destroy()` 现在完全不取这个位置，
而 `stop()` 取过一次之后再调 `destroy()` 会**再取一次**（此刻音频已停，
读到的是回退档，会把好数据覆盖成坏数据）。幂等保护让两条路径随便怎么组合都对。

---

## 四、归属：为什么放 `backends/mediacodec/session/` 而不是新开一族

家族树规则第 2 条：子域文件数 ≥ 3 且职责可再分时才开子目录。这份东西是
**一个头文件**，开新目录必然是个只有一人的空壳。

放 `session/` 的理由是它的内容确实是"会话编排"：这七步讲的全是
`DecodeSession` / `AudioPlayback` / `ANativeWindow` / `AudioClockSource`
四个东西谁先死，而这四个正是 `session/` 已经在编排的那批。

> 唯一别扭的地方：`session_teardown_plan.hpp` 提到了 `AudioPlayback` 与
> `ANativeWindow`，而 `session/` 属 mediacodec 族。**认这个别扭**，
> 因为另一种选择（新开 `runtime/android_session/`）会得到一个 1 文件的目录，
> 违反规则第 2 条 —— 两条规则冲突时，选不制造空壳的那条。
> 等 P12-E 真把 runtime 层铺开、这个文件有 3 个邻居了，再搬。

---

## 五、Kotlin 侧：把 D2 留下的重复收掉

D2 时 `PlaybackSession.kt` 与 `PlayerActivity` 的内联逻辑重复，当时刻意没动
（`08-p12-family-tree.md` 三之八写了"收敛归 D4"）。这次收：

- `PlayerActivity` 保留：生命周期回调、Surface 传递、轮询、证据落盘、extra 解析。
- 全部交给 `PlaybackSession`：`SessionHandle` 的创建与释放、`ParcelFileDescriptor`
  的开与关、`open/start/setPacing/setPipeline/openAudio/writeStats/writeSyncStats/rebind`。

**这是一次纯搬迁，判据是"数字不变"**：收完之后重跑 C8 的 `cpu_paced` 带音轨一轮
与 D1 的换面一轮，`av_drift` / `playback_rate` / 帧数闭合必须与归档证据一致
（口径见 §六）。不一致就回滚，不解释。

为什么敢动：`PlayerActivity` 里那段重复已经出过一次事故 ——
`audioDescriptor` 与 `mediaDescriptor` 分别关，漏关一个就是 fd 泄漏，
而 `PlaybackSession` 里只有一个 `close()`。重复的代价已经付过利息了。

---

## 六、验收计划

### 6.1 单测（新增一个目标，不塞进 pace 测试）

`tests/video/session/p12d4_lifecycle_tests/session_teardown_tests.cpp`，六条：

| # | 断言 | 守的是什么 |
| --- | --- | --- |
| t27 | 销毁计划逐项等于预期七步 | 顺序本身 |
| t28 | 停止计划是销毁计划的严格前缀 | 唯一性（两份列表会各自演化） |
| t29 | 所有 `reset` 步都排在所有 `stop` 步之后 | "先停后放"这条硬约束 |
| t30 | `kSampleAudioPosition` 在 `kStopVideo` 之后、`kStopAudio` 之前 | C8 那个覆盖 bug 的形状 |
| t31 | 用记账钩子跑停止计划 → 记录恰为前三步；跑销毁计划 → 恰为七步 | 执行器没有自己的顺序 |
| t32 | 停止跑两遍 + 销毁跑一遍，采样钩子只生效一次 | 幂等（`destroy` 不覆盖已取到的位置） |

t31 是这套测试的核心：它证明"执行器按数据走"，而不是"数据好看但代码另写一套"。

### 6.2 真机回归（两轮，不新造判据）

| 轮 | 命令 | 必须一致的东西 |
| --- | --- | --- |
| C8 回归 | `av_1080p30_aac.mp4` + `kivo.pace/drop/fps` + `kivo.present cpu_paced` | `av_drift.p99` 仍 < 1 ms、`playback_rate` ∈ [0.98,1.02]、帧数闭合、`clock_source = audio_timestamp` |
| D1 回归 | `h264_1080p8.mp4` + 切后台 1 s 再回来 | `surface_rebuild.rebuilds = 1`、`resume_ms` 在既往量级（几十 ms）、`frames_discarded ≈ 空窗×30` |

不追求数字**逐位相同**（解码耗时本来就有轮间抖动），追求的是：
不变量仍成立、量级不变、没有任何新字段变成 0 或缺省。

### 6.3 明确不做

- **不测"忘记调 stop 直接 destroy"这条路径**（JNI 层没有这个调用方，
  测了也是造出来的场景）。它在计划上是对的（全七步），但不作为验收项。
- **不动 `AudioPlayback` / `DecodeSession` 内部的收尾**。它们各自的幂等保护是
  C8/D1 的产物，这次只碰"谁先谁后"，不碰"每一步里面做什么"。
- **不把 `SessionHandle` 搬进 backend**。搬了要给它造一层平台无关的抽象
  （窗口、时钟、音频全要虚化），为了一个 40 行的结构体引入四层接口，不值。
  这个取舍写进 `32-p12d4-status.md` 的诚实清单。

---

## 七、风险

| 风险 | 处置 |
| --- | --- |
| Kotlin 搬迁改坏取证路径 | 先搬、再跑两轮回归；不一致就回滚，不就地调试 |
| 模板执行器把 JNI 代码变得难读 | 只在 `session_stop`/`session_destroy` 两处用；`Hooks` 定义紧贴 `SessionHandle`，读的人一屏能看到头和尾 |
| `kSessionStopPlanSize = 3` 与数组长度脱钩 | t28 同时断言 `stop_plan().size() == kSessionStopPlanSize`，防止改了数组忘了常量 |

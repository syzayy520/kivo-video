# P12-C4 · 音频主时钟（真机验收）

- 日期：2026-10-29
- 状态：`DONE`
- 证据：`docs/evidence/p12c/p12c4-clock-*.json`（7 份，Redmi K40 `b6e950ee` 实跑）
- 设计：`15-p12c4-design.md`
- 单测：`backend/tests/video/audio_plane/p12c4_clock_tests/`（13 例 / 66 断言）

---

## 一、一句话结论

**零点问题不存在了。** 时钟由"已呈现帧位置"驱动，`framePosition` 在首帧出声前恒为 0，
所以启动开销那 25–147 ms **自动被排除**，不需要任何常数补偿。
七次真机运行全部 `fail=0`、`fallback=0`、`stale=0`、`xrun=0`。

---

## 二、七次真机数据（全部完整播放，无截断）

| run | 片源 | startup | first_ok_frame | rejects | latency | queries | ratio | silenced(头/尾) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 48k run0 | 48k/2ch/10s | 127 ms | 0 | 3 | 2736 | 1969 | 0.9944 | 320（**0 / 320**） |
| 48k run1 | 同上 | 146 ms | 768 | 2 | 2736 | 1963 | 0.9928 | 128（**0 / 128**） |
| 48k run2 | 同上 | 25 ms | 768 | 3 | 2736 | 1964 | 0.9928 | 128（**0 / 128**） |
| 48k run3 | 同上 | 25 ms | 0 | 3 | 2736 | 1967 | 0.9945 | 128（**0 / 128**） |
| 44.1k run1 | 44.1k/1ch/5s | 25 ms | 0 | 8 | 6747 | 971 | 0.9695 | 198（**0 / 198**） |
| 44.1k run2 | 同上 | 147 ms | 0 | 8 | 6747 | 970 | 0.9677 | 198（**0 / 198**） |
| 44.1k run3 | 同上 | 25 ms | 0 | 8 | 6747 | 970 | 0.9680 | 198（**0 / 198**） |

七次共同点（这是比任何单个数字都硬的部分）：

```
timestamp_fail      = 0     主源一次都没失败
fallback(frames_read/frames_written/frozen) = 0/0/0
stale_freezes       = 0
xrun_count          = 0
clamped_jump        = 0
silence_head_frames = 0     头部静音：一帧都没有
```

---

## 三、三条必须记住的结论

### 1. 启动开销是**双峰**，不是连续抖动

七次落点：**25 / 25 / 25 / 25 / 127 / 146 / 147 ms**。

不是均匀分布在 24–147 ms 之间，而是**两档**：快的 ~25 ms，慢的 ~130–147 ms。
C1 当时测到的 146 / 25 / 24 / 146 / 157 同样是双峰，只是当时样本太少没敢下这个判断。
现在 7 次印证了：**冷启动（慢档）与热启动（快档）是两种状态**。
这比"平均多少毫秒"更有意义 —— 它意味着**任何固定常数补偿都会在一半的启动里错 100 ms 以上**。

### 2. 官方文档说"单调递增"，实测**不成立**

NDK 对 `AAudioStream_getTimestamp` 的原文是：

> The position and time passed back are **monotonically increasing**.

实测：1963–1969 次查询里，**48k 有 2–3 次回退，44.1k 有 8 次回退**（每次运行都有）。

概率只有 0.1%–0.8%，但一次就够让 `audio_clock` 倒退，C5 的节拍器会据此把一帧判成
"晚了"并触发无谓追帧 —— 而这种抖动在统计上只表现为 p99 冒尖，极难归因。
**`MonotonicClockGuard` 不是防御性代码，是必需品。** 这条已写进设计文档和代码注释。

另外 44.1k 的回退次数是 48k 的 3 倍（8 vs 2–3），与 latency 的差异同向 ——
44.1k 在设备侧走了不同的处理路径（见结论 3）。

### 3. 管线延迟按采样率分档，44.1k 是 48k 的 2.7 倍

`latency_frames = getFramesWritten − framePosition`，七次运行**完全恒定**：

| 采样率 | latency_frames | 折算 |
| --- | --- | --- |
| 48000 | 2736 | **57.0 ms** |
| 44100 | 6747 | **153.0 ms** |

57 ms 已经超过 G7 的 40 ms 阈值；153 ms 是它的近 4 倍。
**这不是时钟的误差，而是设备的物理管线深度**：`audio_clock` 报的是"已经播出去的"，
而"刚提交进去的"要等 57/153 ms 才播。C5 计算 `due_at` 时必须知道这个值，
否则会系统性提前排帧。

---

## 四、C3 遗留问题结案：静音全在尾部

C3 留下的疑问是"`frames_silenced` 128/320/198 帧分不清头还是尾"。加上
`first_audible_frame` 之后一次判死：

| 片源 | frames_silenced | first_audible_frame | 头部 | 尾部 |
| --- | --- | --- | --- | --- |
| 48k run0 | 320 | 0 | **0** | 320 |
| 48k run1–3 | 128 | 0 | **0** | 128 |
| 44.1k run1–3 | 198 | 0 | **0** | 198 |

**七次全部 `silence_head_frames = 0`。** 结论：

1. 头部一帧静音都没有 → **C3 的预灌（3 个 burst）是足够的**，起流瞬间没有欠载；
2. 静音全部落在尾部 → 是"收尾时环已空、流还在要数据"的正常排空，不是供给故障；
3. 帧数在 128–320 之间波动（2.7–6.7 ms），符合"收尾时机随调度浮动"的解释。

这一条不解决，G7 的 40 ms 阈值就没有干净基线 —— 现在基线是干净的：
**稳态播放期间零静音，全部静音集中在最后 3–7 ms。**

---

## 五、`span_ratio` 0.993 / 0.968 怎么解释（不是时钟不准）

`clock_span`（时钟走了多久）比 `rendered_span`（交出去的帧本该播多久）短：

| 采样率 | rendered_span | clock_span | 差 | latency |
| --- | --- | --- | --- | --- |
| 48k | 10.008 s | 9.936 s | 72 ms | 57 ms |
| 44.1k | 5.020 s | 4.858 s | 162 ms | 153 ms |

差值的主要部分正是 latency：**交出去的帧里，最后 57 ms（或 153 ms）还在管线里没播**，
所以时钟当然不会走到那里。剩余 9–15 ms 与尾部静音（2.7–6.7 ms）及收尾取数顺序同量级，
未逐项归因，记入"已知未做"。

**判据**：如果时钟是靠墙钟硬凑的，这个比值会随运行时长持续拉大；
实测七次都在 0.968–0.995 且不随时长漂移，说明时钟确实跟着设备走。

---

## 六、家族树落位（与设计文档一致）

```
backend/include_private/video/video_plane/
├── core/audio/clock/                     ← 跨平台纯逻辑，桌面可单测
│   ├── audio_clock_types.hpp / .cxx      来源枚举 + 快照 + 可读名
│   ├── audio_clock_source.hpp            注入接口（C5 只依赖这一层）
│   ├── monotonic_clock_guard.hpp / .cxx  单调守卫
│   ├── frame_position_extrapolator.hpp / .cxx  外推 + 采样率切换
│   └── system_clock_source.hpp / .cxx    无音轨时的独立时钟
└── backends/aaudio/clock/                ← Android 平台层
    ├── aaudio_timestamp_reading.hpp / .cxx  读取结果 + 状态分类
    ├── aaudio_clock_probe.hpp               读数接口（为了能单测回退分级）
    ├── aaudio_timestamp_reader.hpp / .cxx   唯一调 AAudio 时钟 API 处
    ├── aaudio_clock_stats.hpp               计数 + dominant_source
    ├── aaudio_monotonic_now.hpp / .cxx      CLOCK_MONOTONIC 取时刻
    └── aaudio_clock_source.hpp / .cxx       编排：读→外推→守卫→快照
```

与任务卡的偏差及理由（两条，均在设计文档 §5/§6.2 展开）：

| 卡片写的 | 实际落位 | 理由 |
| --- | --- | --- |
| `clock_monotonicity_guard` 在 `backends/aaudio/clock/` | `core/audio/clock/monotonic_clock_guard` | iOS AudioUnit 同样需要外推+守卫；放后端等于宣布只属 Android（同 C2 把环形缓冲移到 core 的理由） |
| 回退用 `getFramesWritten` | 三档：`getFramesRead` → `getFramesWritten − latency` → 冻结 | 直接用 `getFramesWritten` 会超前整整一个缓冲深度（实测 57/153 ms），同步器会立刻追帧 |

---

## 七、单测（桌面，无需设备）

```
=== P12-C4 AaudioClockSource tests ===
[PASS] C1 时间戳倒退被拒绝
[PASS] C2 时间戳跳变被钳制
[PASS] C2b 正常推进不被误伤
[PASS] C3 切换采样率位置连续
[PASS] C4 外推加入经过时间
[PASS] C5 陈旧时间戳停止外推
[PASS] C6 启动期返回 0 不计失败
[PASS] C7 首个成功读数不被钳制
[PASS] C8 回退档1 getFramesRead
[PASS] C9 回退档2 减 latency
[PASS] C10 三档全废则冻结
[PASS] C11 主导来源反映真实
[PASS] C12 系统时钟单调且自报
--- 断言 66 项，失败 0 项 ---
```

`ctest -R kivo_p12c` 3/3 通过（环缓冲、PCM 转换、时钟），C2/C3 无回归。

### 单测里两个刻意设计的用例

- **C7「首个成功读数不被钳制」**：启动期若用 0 给守卫打基准，首个真实读数
  （设备可能已播 250 ms）会撞上 200 ms 阈值被钳制，凭空砍掉真实进度。
  所以启动期**不喂守卫**，守卫的第一次观测必须留给真正的采样点。
- **C9「回退档 2 减 latency」**：用 `written=2304 / latency=1536` 构造场景，
  断言输出是 768 帧（16 ms）而不是 2304 帧（48 ms）。不减 latency 的写法会在这里当场红。

---

## 八、复现

```bash
CXX=/c/Android/sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android26-clang++
ADB=/c/Android/sdk/platform-tools/adb.exe

cd /c/kivo video/android
JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug --no-daemon

# 探针在 Gradle 产物里，不在 build/p12c/ 下
P="C:/kivo video/android/app/build/intermediates/cxx/Debug/<hash>/obj/arm64-v8a/kivo_audio_file_probe"
MSYS_NO_PATHCONV=1 $ADB push "$P" /data/local/tmp/
MSYS_NO_PATHCONV=1 $ADB shell 'chmod 755 /data/local/tmp/kivo_audio_file_probe'   # push 会重置权限
MSYS_NO_PATHCONV=1 $ADB shell '/data/local/tmp/kivo_audio_file_probe /data/local/tmp/aac_48k_stereo_lr.mp4 > /data/local/tmp/o.json 2>/data/local/tmp/o.err'
MSYS_NO_PATHCONV=1 $ADB pull /data/local/tmp/o.json docs/evidence/p12c/
```

单测（桌面）：

```bash
cd /c/kivo video
unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY
cmake --build build/vs-debug --target kivo_p12c4_clock_tests --config Debug
./build/vs-debug/backend/Debug/kivo_p12c4_clock_tests.exe
```

---

## 九、已知未做（归属明确）

| 项 | 归属 / 理由 |
| --- | --- |
| 用 `audio_clock` 做 A/V 同步决策（追帧/丢帧） | C5 呈现节拍器。C4 只提供时钟，不做决策 |
| seek 后时钟重建 | P12-F。当前的跳变钳制只是防御，不是 seek 方案 |
| `span_ratio` 剩余 9–15 ms 未归因 | 与尾部静音、收尾取数顺序同量级；不构成结论风险，需要时再查 |
| 流复用（stop 后再 start，`framePosition` 不回零） | P12-F。C4 每次都是新开流，位置从 0 起 |
| 44.1k 为何延迟 153 ms | 属设备/HAL 行为，本轮只记录现象。若 C5 在 44.1k 上对不齐，从这里查起 |

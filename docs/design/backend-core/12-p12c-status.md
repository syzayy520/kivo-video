# P12-C1 · AAudio 输出骨架

> 状态：**已闭环 —— 数据链路有真机证据，物理出声已由人耳确认**
> 证据：`docs/evidence/p12c/aaudio-tone-redmi-k40.json`（3s 440Hz）、
> `aaudio-tone-redmi-k40-10s.json`（10s 1kHz）、`aaudio-tone-listening-redmi-k40.json`（5s 440Hz，人耳验收轮）
> 设备：Redmi K40（alioth / SM8250 / Android 13 API 33）
> 日期：2026-10-29

> **人耳验收结论（2026-10-29 06:03）**：
> 用户在真机上当场确认「有声音」。这一条不是自动化能得出的结论，必须由人给出。
>
> 自动化能证明的只到「样本确实交给了 AAudio 并持续被设备取走」——
> `frames_rendered` 与期望值吻合、`frames_silenced = 0`、`xrun_count = 0`。
> 三项同时成立只能说明数据进了音频 HAL 且供给没断流，
> **不能排除**"进了 HAL 但 HAL 出口被静音/路由到不存在的设备"这类失败。
> 所以验收必须两段：客观指标由脚本断言，出声由人耳断言。缺一段都不算过。

---

## 一、做成了什么

从「只有画面」到「音频链路也通了」：

```
探针进程 → AaudioToneSource（正弦）
              ↓ SampleSource 接口
         AaudioOutputSession ──回调──→ AAudio ──→ 音频设备
              ↑
         AaudioStreamOpener（builder → open，含降级链）
```

关键点：**会话不知道声音是什么**。它只知道"向 source 要 frames，要不到的补静音并计数"。
P12-C2 接环形缓冲时，只需让缓冲实现 `SampleSource`，会话一行都不用改。

---

## 二、实测数据

| 场景 | 时长 | 帧数 | 期望帧数 | ratio | xrun | 补静音 | 回调数 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 440Hz 3s | 3.146 s | 146,304 | 144,000 | 1.016 | **0** | **0** | 810 |
| 1kHz 10s | 10.146 s | 482,496 | 480,000 | 1.005 | **0** | **0** | 2,670 |

协商结果（两次一致）：

| 项 | 请求 | 实际 | 结论 |
| --- | --- | --- | --- |
| 采样率 | 48000 | 48000 | ✅ |
| 声道 | 2 | 2 | ✅ |
| 格式 | pcm_float | pcm_float | ✅ |
| 性能模式 | low_latency | low_latency | ✅ |
| **共享模式** | **exclusive** | **shared** | ❌ **未拿到**，已记入 `downgrade_reason` |
| frames_per_burst | — | 192（4 ms @48k） | 低延迟生效 |
| buffer_capacity | — | 1536（32 ms） | 合理 |

`xrun = 0` 且 `frames_silenced = 0` 说明供给始终跟得上，没有欠载。
回调粒度 180.7 帧/次（≈3.76 ms），足够支撑 C5 节拍器做同步判断。

> 上表是两次代表性运行。五次完整测量（含启动偏移的离散度）见 §三·发现 2 ——
> 那里的数据推翻了本节初稿里"启动偏移恒定"的错误结论，请不要只看这张表就下判断。

---

## 三、三个发现，每个都会影响后续

### 发现 1：EXCLUSIVE 是「提示」，不是契约

请求独占、`openStream` 返回 OK、`getSharingMode` 却是 **shared**。
第一次跑出来时 `downgrade_reason` 是空的 —— **我们自己的检测漏了**。

当时只校验了格式、采样率、声道，没校验共享模式与性能模式。
这个漏洞的后果很具体：独占与共享的延迟差一个量级，
如果按"我申请了独占"去做同步预算，G7 的 40ms 会莫名其妙卡在边缘。

已修：`append_negotiation_notes` 现在同时校验 sharing_mode 与 performance_mode，
不符即写入 `downgrade_reason`。修完重跑，证据里如实出现「申请独占但实际为 shared」。

> 这是 P12-C1 最有价值的一条：**探针的价值不是证明能跑，而是逼出"我们以为的事"和"实际的事"之间的差距。**

### 发现 2：启动开销与首帧偏移都**不是常数**，因此禁止任何固定补偿

先把全部测量摊开（`wall` = 从 `requestStart` 前到取统计；帧偏移 = 实际帧数减期望帧数）：

| 请求时长 | wall | wall 差值 | 帧数 | 期望 | 帧偏移 |
| --- | --- | --- | --- | --- | --- |
| 3 s | 3146 ms | **+146 ms** | 146,304 | 144,000 | +48 ms |
| 3 s | 3025 ms | **+25 ms** | 145,728 | 144,000 | +36 ms |
| 5 s | 5024 ms | **+24 ms** | 242,112 | 240,000 | +44 ms |
| 5 s | 5157 ms | **+157 ms** | 242,304 | 240,000 | +48 ms |
| 10 s | 10146 ms | +146 ms | 482,496 | 480,000 | +52 ms |
| 2 s | — | — | 98,496 | 96,000 | +52 ms |

> 存档对应：第 1 行 = `aaudio-tone-redmi-k40.json`，第 4 行 = `aaudio-tone-redmi-k40-10s.json`，
> 第 3 行 = `aaudio-tone-listening-redmi-k40.json`（人耳验收那一轮）；
> 其余为本轮现场复测（同一二进制、同一设备、同一参数），未单独存档。
> 第 1 行与第 2 行是同一配置连跑两次，正是这两行推翻了"恒定偏移"的说法。

两列都不稳定：wall 差值在 **24–157 ms**，帧偏移在 **36–52 ms**。
注意第 3 行与第 4 行是同一个 5 s 配置，wall 差值一个是 +24 ms 一个是 +157 ms ——
**同一配置、同一设备、隔几分钟，差值差 6 倍**。这是"禁止常数补偿"最直接的证据。

> **过程记录（比结论更值得留）**：最初只测了 3 s 与 10 s 两个点，两次 wall 差值都是 146 ms，
> 于是当场写下了「恒定 146 ms 启动偏移」。第三次测量（3 s 重跑）给出 25 ms，结论立即被推翻。
> 两次相同的数字**不构成规律**，尤其在这种带冷启动的路径上。
> 这也说明本节的表格为什么必须列满五次测量，而不是只留"代表性"的两行。

对 C4 的三条硬要求：

1. **音频时钟零点必须取"首帧实际出声"**，不能取 `requestStart` 返回的时刻。
   中间隔着状态机推进与硬件缓冲填充，量级几十毫秒 —— 与 G7 的 40 ms 阈值同阶。
2. **禁止用固定常数补偿**（无论是 146 ms 还是 48 ms）。偏移本身有 20–120 ms 的抖动，
   任何常数都会在某些次运行里把误差推到阈值之外。
3. **必须用 `AAudioStream_getTimestamp` 读真实帧位置**来建立时钟，
   而不是用 `framesWritten / sampleRate` 估算，更不是用墙钟减常数。

首次运行（冷启动）明显最慢（146 ms），热启动约 24 ms。这个差异本身就是"不能补偿"的证据。

### 发现 3：回调里绝对不能有任何阻塞

第一版很自然地想在回调里更新统计，但那是实时音频线程。
本实现里回调只做三件事：原子加计数、调一次 `source->render`、不足部分 `std::fill_n` 补零。
没有锁、没有 new、没有日志、没有字符串。10 秒 2670 次回调零 xrun 是这个约束生效的直接证据。

---

## 四、家族树落位（实际实现）

```
backends/aaudio/
├── device/
│   ├── aaudio_stream_config.hpp         请求参数（纯数据，零平台依赖）
│   ├── aaudio_open_outcome.hpp          协商结果 + 降级原因（纯数据）
│   ├── aaudio_format_names.{hpp,cxx}    平台枚举 → 可读名（纯函数）
│   └── aaudio_stream_opener.{hpp,cxx}   builder → open + 降级链
└── session/
    ├── audio_output_stats.hpp           统计事实（纯数据）
    └── aaudio_output_session.{hpp,cxx}  启停 + 数据回调 + 统计

probe/audio/
├── aaudio_tone_source.{hpp,cxx}         正弦源（实现 SampleSource）
├── aaudio_probe_report.{hpp,cxx}        报告 → JSON（纯函数）
└── aaudio_tone_main.cxx                 入口
```

**刻意的边界**：正弦波不在后端里。一旦 `backends/aaudio` 开始自己造波形，
接真实解码器时就得先把它拆掉。测试信号属于探针域。

---

## 五、一处编译期教训

第一版凭记忆写了四个不存在的常量名，全部当场被编译器拦下：

| 我写的 | 实际 | 说明 |
| --- | --- | --- |
| `AAUDIO_FORMAT_PCM_24_BIT_PACKED` | `AAUDIO_FORMAT_PCM_I24_PACKED` | I 前缀 |
| `AAUDIO_FORMAT_PCM_32_BIT` | `AAUDIO_FORMAT_PCM_I32` | 同上 |
| `AAUDIO_FORMAT_PCM_8_24_BIT` | 不存在 | 删除 |
| `AAUDIO_ERROR_UNEXPECTED_STATE` | 不存在 | 删除 |
| `AAudioDataCallback` | `AAudioStream_dataCallback` | 类型名前缀是 `AAudioStream_` |

`aaudio_format_names.cxx` 现在是照着 r30 的 `aaudio/AAudio.h` 逐个抄的，
不再是印象。**平台枚举一律以头文件为准，不许凭记忆写。**

---

## 六、复现

```bash
# 1. 直接交叉编译（不经 Gradle，迭代快；CMake 里也已正式注册）
cd "/c/kivo video"
CXX=/c/Android/sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android26-clang++
$CXX -std=c++23 -O2 -static-libstdc++ -Wall -Wextra -Wpedantic -Wshadow \
  -I backend/include_private \
  backend/src/video/video_plane/backends/aaudio/device/aaudio_format_names.cxx \
  backend/src/video/video_plane/backends/aaudio/device/aaudio_stream_opener.cxx \
  backend/src/video/video_plane/backends/aaudio/session/aaudio_output_session.cxx \
  backend/src/video/video_plane/probe/audio/aaudio_tone_source.cxx \
  backend/src/video/video_plane/probe/audio/aaudio_probe_report.cxx \
  backend/src/video/video_plane/probe/audio/aaudio_tone_main.cxx \
  -laaudio -llog -o build/p12c/kivo_audio_probe

# 2. 部署（chmod 必须每次重做：adb push 会重置权限）
ADB=/c/Android/sdk/platform-tools/adb.exe
MSYS_NO_PATHCONV=1 $ADB push build/p12c/kivo_audio_probe /data/local/tmp/kivo_audio_probe
MSYS_NO_PATHCONV=1 $ADB shell "chmod 755 /data/local/tmp/kivo_audio_probe"

# 3. 跑（参数：时长秒 频率Hz）；stdout 只有 JSON，stderr 是人读摘要
MSYS_NO_PATHCONV=1 $ADB shell \
  "/data/local/tmp/kivo_audio_probe 3 440 > /data/local/tmp/p12c.json 2>/data/local/tmp/p12c.err"

# 4. 取证据
MSYS_NO_PATHCONV=1 $ADB pull /data/local/tmp/p12c.json docs/evidence/p12c/<name>.json
```

返回码：`0` 正常出声 / `1` 开流失败 / `2` 启动失败 / `3` 有流但零帧。

> 踩坑记录：`adb push` 会重置文件权限，覆盖后忘记 `chmod 755` 会得到
> `can't execute: Permission denied`（rc=126），很容易误判成"程序崩了"。

---

## 七、已知未做（归属明确，不算遗漏）

| 项 | 归属 |
| --- | --- |
| 环形缓冲（当前直接由 source 供数，无缓冲层） | **P12-C2 已完成**，见 `13-p12c2-status.md` |
| 音频解码（现在只有正弦，没有真实音轨） | P12-C3 |
| 音频主时钟（当前未暴露任何时钟） | P12-C4 |
| 独占模式为何拿不到（系统策略/其他应用占用） | 记为观察项，P12-C 收口时复测 |
| 音频 PCM 格式转换（S16→Float） | P12-C3 |

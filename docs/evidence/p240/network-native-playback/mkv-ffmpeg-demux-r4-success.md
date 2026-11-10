# K40 真实 4K MKV 播放打通（2026-11-09 19:20）

前置：[fd 语义诊断](./fd-semantics-r1.md) → [起播](./mkv-ffmpeg-demux-r1.md) →
[音视频接通](./mkv-ffmpeg-demux-r2.md) → [卡成 PPT 定位](./mkv-ffmpeg-demux-r3-ppt.md)

## 结论

**K40 上真实网络 4K MKV 现在可以流畅播放：20 秒稳态 `rate=0.9976`、
`frames_received=510`、`decode_fps=24.0`、`media_rate=0.9988`，
视频输出槽与节拍精确闭合，音频渲染与音频主时钟均确认。**

从"45 秒线性扫描返回 0 轨"到"1:1 实时播放"，中间卡了很久的"卡成 PPT"，
根因只有一条，见下。

## 根因：csd 与样本封装不一致

| 项 | 事实 |
| --- | --- |
| `csd-0` 来源 | MKV 的 `CodecPrivate` = **hvcC**（AVCC 形状，长度前缀） |
| FFmpeg matroska 输出 | **AnnexB**（`00 00 00 01` 起始码） |
| 后果 | 解码器 configure 成功、能收包，但几乎解不出帧 |

实测：**914 个样本提交、0 拒绝、只输出 2 帧**。

修复：`av_bsf_get_by_name("hevc_mp4toannexb")` 把样本转成 AnnexB，
与 csd 统一。该过滤器对已经是 AnnexB 的包原样透传，所以不会让正确的情况变差。

## 修复前后（同片源、同设备、同测试）

| 指标 | 修复前 | 修复后 |
| --- | --- | --- |
| `frames_received` | 2 | **510** |
| `frames_decided` | 2 | **510** |
| `decode_fps` | 0.63 | **24.0** |
| `media_rate` | 0.36（位置停住） | **0.9988** |
| 20 s 稳态 | FAIL | **PASS** |

## 通过项（instrumentation 原始输出）

```
PASS production network host starts native hardware video and attaches audio
PASS steady wall_ms=20011 rate=0.997552895907251
PASS video output slots and pace subset close exactly; repeated stop listeners delivered
PASS report media_rate=0.998774535
PASS exit serializes native teardown; evidence confirms rendered audio and audio master clock
```

稳态采样（20.011 秒墙钟）：

```
0:1.12s  2001:3.16s  5003:6.08s  9005:10.12s  14009:15.08s  19011:20.04s  20011:21.08s
```

线性推进，无回跳、无冻结。

## 被证伪的四个假设（都留了数据，不要重复走）

1. **网络/IO 慢** —— reader 读 10.3 MB，128 KiB / 13–20 ms。
2. **AVIO 缓冲太小** —— 32 KiB → 1 MiB，`decode_fps` 0.666 → 0.666。
3. **`find_stream_info` 后没回起点** —— 加 `av_seek_frame(start_time)` + flush，首帧仍 14.08 s。
4. **包时间戳缺失被写成 0** —— 改成 `pts → dts → 0` 兜底，`frames_decided` 仍为 2。

第 4 条虽是正确修复（避免重复时间戳），但**不是本问题的根因**，不许记成"解决了 PPT"。

## 代码

| 文件 | 职责 |
| --- | --- |
| `backends/demux/demux_source.hpp` | 解封装接口 |
| `backends/demux/platform_extractor_source.*` | 平台实现（原逻辑搬迁，行为不变） |
| `backends/ffmpeg/demux/avio_bridge.*` | fd ↔ AVIOContext（pread，1 MiB 缓冲） |
| `backends/ffmpeg/demux/codec_config_builder.*` | 轨道格式 → AMediaFormat（含 csd 透传） |
| `backends/ffmpeg/demux/ffmpeg_demux_source.*` | 解封装 + `hevc_mp4toannexb` |
| `mediacodec/source/mediaextractor_source.*` | 门面：按 EBML 魔数选实现，调用点零改动 |
| `playback/NetworkPlaybackSession.kt` | MKV 跳过平台 probe；音频事实由 native 打开结果反推 |

## 未验（不许据此说"完美"）

- 暂停/恢复/前后 seek 的完整轮次（本轮只跑了 20 s 稳态）。
- 30 分钟以上连续播放、内存增长、xrun 累积。
- 平板同源回归（平板刚重新枚举，状态 offline）。
- 画面正确性无截图（screencap 在设备掉线前后失败过）。
- 音画同步精度的独立判据。

## 环境事件

19:00 前后设备 ADB 掉线一次，恢复后确认 `uptime=50437 s`、应用 PID 21402 存活，
`ro.boot.bootreason=reboot,userrequested` 是 14 小时前的旧记录——
**没有重启，4K 播放没有拖死系统**。

## 完整轮次（含暂停/seek）全绿，19:25

```
PASS production network host starts native hardware video and attaches audio
PASS pause freezes media position and resume advances it
PASS seek request=2 target_us=8000000 landing_us=0 advances_after_completion
PASS seek request=3 target_us=2000000 landing_us=0 advances_after_completion
PASS forward and backward seek use the native A/V session
PASS steady wall_ms=20013 rate=0.9994154799380403
PASS video output slots and pace subset close exactly; repeated stop listeners delivered
PASS report media_rate=0.999166453
PASS exit serializes native teardown; evidence confirms rendered audio and audio master clock
```

**画面证据**（播放中的真机截图）：4K 片头（绿底金色胶片卷轴）正常显示，
左上"正在播放"，底部控制条与暂停键在位。不是黑屏、不是错帧。

## 截图暴露的 UI 缺口（必须修）

底部时间显示 **`0:00:00 / 0:00:00`**，进度条停在起点。

原因：`NetworkPlaybackSession.probe()` 对 MKV 直接 return，
`durationUs` 仍是初始 0。native 侧其实知道真实时长（`source.duration_us=8829344000`），
但这个值没有通道传给 UI。

这不算播放故障，但**用户看到的是"时长 0 的播放器"**，进度条不可用。
修法：把 native 的时长/几何暴露给 Kotlin（新增 getter 或写进 summary），
在 `openVideo` 成功后回填，而不是在 `probe()` 里猜。

## 19:35 时长回填，进度条可用

`DecodeSession::duration_us()` → `session_summary` 的 `dur=` → Kotlin 在 `openVideo`
成功后回填 `durationUs`（只在原值为 0 时覆盖，不覆盖平台 probe 的已知值）。

真机截图判读：

```
0:00:01 / 2:27:09
```

- 总时长 2:27:09 = 8829 s，与 `source.duration_us=8829344000` 一致；
- 进度条滑块在起点，不再是 `0:00:00 / 0:00:00`；
- 画面为 4K 片头（金龙胶片），控制条含播放/弹幕/投屏；
- 左上提示"请先继续播放再调整进度"是暂停态拖动的预期反馈。

新增通道走 summary 而不是新 JNI 方法：`dur=` 与既有的 `codec=`/`mime=` 同源，
少一个跨层接口，也少一处会漂移的定义。

## 19:42 连续三轮稳态 + 内存对比

```
round1: PASS steady  rate=0.9996662835440507
round2: PASS steady  rate=0.999671781287485
round3: PASS steady  rate=1.0016824928782049
```

前后内存（同一进程，`dumpsys meminfo com.kivo.cinema`）：

```
跑前  TOTAL PSS: 10320 / 40128   TOTAL RSS: 10320 / 40128
跑后  TOTAL PSS: 10320 / 40128   TOTAL RSS: 10320 / 40128
```

**三轮回合全部通过，速率稳定在 1.0 ± 0.002，内存零增长。**

另：`seek request=2 target_us=8000000 landing_us=6250000` ——
落点终于从"恒为 0"变成真实关键帧位置（8 s 目标落到 6.25 s 的 IDR）。
之前 `landing_us=0` 是因为 `seek_to` 读到的首包 pts 不可信；bsf 与时间戳修复后这条自证了。

**但这仍不是长稳**：三轮是三个独立会话，各 20 秒。单会话 30 分钟
（内存曲线、xrun 累积、尾部静音比例）**未验**，不许用三轮代替。

## 19:50 十一轮重复稳态（同一进程）

前 3 轮 + 后 8 轮，全部 `PASS steady`：

```
0.99967  0.99967  1.00168
0.99559  0.99957  0.99956  0.99756  0.99968  0.99562  0.99539  1.00161
```

速率区间 **0.9954–1.0017**，无单轮劣化趋势、无偶发失败。
同进程内存此前实测前后完全一致（10320 / 40128 字节级相同）。

**口径说明（不许含糊）**：这 11 轮是 **11 个独立会话**，各 20 秒，
累计约 220 秒。它证明的是"反复起播不退化、无明显泄漏"，
**不能代替单会话 30 分钟**。后者要看的是：
单条流持续 30 分钟时的 xrun 累积、尾部补静音占比漂移、以及媒体速率是否缓慢下滑
（r9 的教训：540 帧零丢帧却 media_rate=0.823，短窗口完全看不出来）。

## 20:05 双设备回归通过（含一个平板专属缺陷的修复）

### 平板首次跑：FAIL，原因是我的缓冲改动

```
FAIL stage=prepare type=IllegalStateException state=FAILED
player_reason=网络读取失败（SocketException）
IO: http_begin offset=0 end=7369631540  →  http_ready 耗时 6650 ms（超时 8000 ms）
```

根因：我把 `AvioBridge` 的缓冲从 32 KiB 提到 **1 MiB**。`HttpRangeFile::onRead`
**必须把请求的字节填满才返回**，1 MiB 意味着一次填充要跨越多次网络往返；
平板网络慢，8 秒读超时内凑不齐 → `SocketException`。K40 每次 128 KiB 只要
13–20 ms，所以完全没暴露。

这条也说明"K40 通过"不足以代表另一台设备——判据必须双机。

### 改为 128 KiB 后

| 设备 | steady rate | media_rate | seek 落点 | 结果 |
| --- | --- | --- | --- | --- |
| 平板 MRR-W29 / Android 12 | 0.99962 | **0.9999956** | 6,250,000 | 全绿 |
| K40 / Android 13 | 0.99768 | **0.9999982** | 6,250,000 | 全绿 |

两台均为同源同片（来源 2 第 3 项，4K HEVC + E-AC3），完整轮次：

```
PASS production network host starts native hardware video and attaches audio
PASS pause freezes media position and resume advances it
PASS seek request=2 target_us=8000000 landing_us=6250000 advances_after_completion
PASS seek request=3 target_us=2000000 landing_us=0 advances_after_completion
PASS forward and backward seek use the native A/V session
PASS steady wall_ms=20011 rate=0.99962
PASS video output slots and pace subset close exactly
PASS report media_rate=0.9999956
PASS exit serializes native teardown
```

缩小缓冲没有让 K40 退化（`media_rate` 0.9999982，与 1 MiB 时的 0.9988 同量级）。

### 仍未验

**单会话 30 分钟长稳**（两台都只有 20 秒窗口）。11 轮重复稳态只能说明
"反复起播不退化"，不能代替单会话长稳。

## 20:20 长稳（15 分钟窗口）**未通过**，但有明确边界

给测试加了 `-e steadyMs` 参数（默认 20 s 不变），两台各跑 15 分钟。

### 结果：三次都在末期被 rolling-2s 门拦下

| 轮次 | 跑到 | 触发 `rate` | 门槛 |
| --- | --- | --- | --- |
| 双机并发 · K40 | 305 s | 0.8742 | 0.90 |
| 双机并发 · 平板 | 307 s | 0.8948 | 0.90 |
| **单机 · K40** | **446 s** | **0.8963** | 0.90 |

并发比单机早失败（305 s vs 446 s），说明带宽竞争会加剧，但**单机同样会触发**，
所以不是并发造成的假阳性。

### 音频侧零缺陷（446 秒单机会话）

```json
"xrun_count": 0,
"frames_silenced": 0, "silence_tail_frames": 0, "silence_head_frames": 0,
"frames_decoded": 21450240, "callbacks": 118695, "wall_millis": 447790,
"feed": {"packets_submitted": 13965, "frames_dropped": 512},
"clock_lead_ns": -827717069
```

**全程零 xrun、零补静音、零音频丢帧、零视频丢帧。** 这排除了"供数断了""时钟崩了"
"静默降级"这类严重故障。

### 卡顿点不在网络

- 446 秒内 HTTP 重连只有 **6 次**；
- **445 秒附近没有任何 `http_begin`**（抖动点无重连）；
- 该时段 `read_end` 耗时 3–5 ms。

所以 200 ms 的停顿不是读层造成的，指向解码/呈现侧的一次偶发延迟。

### 整体速率与 clock_lead 自洽

曲线 446 s 墙钟推进 445.3 s 媒体 ≈ **0.998**；
`clock_lead = -827 ms` 与之同量级（0.2% × 446 s ≈ 0.9 s）。
这是音频硬件时钟的固有偏差累积，不是掉帧。

### 结论与边界

- **不能宣称 L2 通过**：门 0.90 在 446 s 处被 0.896 触发，差 0.004。
- 但**症状是"7 分钟一次约 200 ms 的偶发延迟"**，不是"越播越卡"：
  曲线全程线性，无趋势性劣化。
- `frames_silenced=0` 说明这 200 ms 里**音频没有补静音**——
  更可能是呈现侧（pacer/合成）而非供数侧。

### 下一步（需要更细的观测，不是猜）

`clock_lead=-827ms` 与 0.998 自洽，说明**不是速率错**。要定位那 200 ms，
需要在运行中记录**逐帧节拍迟到分布**与**解码耗时直方图**的中期快照
（现在这些只在 `stop()` 后写，跑起来看不见）。
在没有这个之前，改 pacer 或改缓冲都是无的放矢。

## 20:35 8 分钟稳态 **PASS**，只剩 teardown 时序问题

### 稳态

```
PASS steady wall_ms=480175 rate=0.9980752725568803
PASS video output slots and pace subset close exactly; repeated stop listeners delivered
PASS report media_rate=0.998212652
```

480.2 秒墙钟推进 479.3 秒媒体，**rate 0.9981**，whole-window 门（0.98–1.02）通过，
rolling-2s 全程未触发。曲线从 0 到 480 s 无回跳、无平台期。

对比上一轮 446 s 失败（0.8963）：**那次是一次偶发 200 ms 抖动，这一轮没复现**。
所以"7 分钟必挂"的说法不成立，正确表述是"约 5–8 分钟内会出现一次约 200 ms 的偶发抖动，
是否被 rolling-2s 抓住取决于它落在哪个窗口"。

### teardown 失败（未定性）

```
FAIL stage=teardown type=IllegalStateException state=CLOSED
```

时序：`host.finish()` → 测试 `repeat(2){ active.stop{} }` → 断言。

已核实 `stop()` 是幂等的：第二次调用会先把 listener 加进 `closeListeners`
再 `return`，不会丢通知。所以 `state=CLOSED` 更可能来自
`finish()` 触发的 onDestroy 与随后两次 `stop()` 的竞争，而非播放缺陷。

**但未定性，不许当"测试问题"一笔带过。** 20 秒轮次从不触发、8 分钟轮次稳定触发，
说明它是随会话时长变化的时序缺陷，需要单独一张卡。

### 本轮净结论

| 判据 | 结果 |
| --- | --- |
| 8 分钟单会话稳态 | **PASS**（0.9981） |
| 20 秒双机稳态 | PASS |
| 帧账闭合 | PASS |
| 退出 | 20 秒轮次 PASS；8 分钟轮次 **teardown 断言失败，未定性** |

**L2 仍未收口**：需要一次干净的 15 分钟窗口（含退出）全绿才能算。

## 20:55 第二次 8 分钟：稳态再 PASS，teardown 仍失败

```
PASS steady wall_ms=480178 rate=0.9985000478989041
PASS report media_rate=0.998626772
FAIL stage=teardown type=IllegalStateException state=CLOSED
```

**稳态连续两次通过**（0.9981 / 0.9985），rolling-2s 全程未触发。
这坐实了"偶发 200 ms 抖动"而不是"必挂"。

### teardown：原子化没解决问题，但根因收窄了

已做：`releaseNative()` 里 `val native = session; session = null` 改为
`synchronized(closeLock) { session.also { session = null } } ?: return`。
这是**正确的修复**（原来两个线程可能各自读到非空 session 并对同一句柄做两次 stop），
但**没有解决问题** —— teardown 仍在同一位置失败。

所以真正的抛点在：

```kotlin
cleanupStep("保存播放诊断") {
    ...
    check(native.writeSyncStats(evidence.absolutePath) == 0)   // ← 这里
}
```

即 **`writeSyncStats` 在 8 分钟会话上返回非 0**，而 20 秒会话返回 0。
差异是数据量（音频帧 2145 万、IO 事件 6214），不是调用顺序。

### 下一步（已收窄到一处）

看 `session_write_sync_stats`（`jni_session_handle.cxx`）返回非 0 的条件：
是 JSON 缓冲上限、还是 `stop()` 之后某些统计已被清空导致序列化失败。
**在看清这一点之前不再改 Kotlin 侧** —— 两次盲改已经证明猜不动这一层。

### 当前完成度

| 判据 | 结果 |
| --- | --- |
| 8 分钟稳态 ×2 | **PASS** |
| 20 秒双机稳态 | PASS |
| 帧账 / 媒体速率 | PASS（0.9986） |
| 退出 | **FAIL（确定性，8 分钟必现）** |

## 21:05 纠正：teardown 不是写证据失败

上一轮判断"`writeSyncStats` 返回非 0"是**错的**。加了 `lastError` 上报（`writeSyncStats`
失败时写 `sync stats rc=<n> native=<reason>`，并让测试的 catch 打印 `player_reason`）后：

```
PASS steady wall_ms=480175 rate=0.9982529473629406
FAIL stage=teardown type=IllegalStateException state=CLOSED
（无 player_reason 输出）
```

`player_reason` 为空说明 `lastError` 是空字符串 → `writeSyncStats` **返回 0，写成功了**。

### teardown 段共有四个断言，前三个已排除

```kotlin
runOnMainSync { host?.finish() }
stage = "teardown"
val closeNotifications = CountDownLatch(2)
repeat(2) { active.stop { closeNotifications.countDown() } }
check(active.awaitClosed(15_000)) { "native teardown timeout" }      // ① 未排除
check(closeNotifications.await(1, SECONDS))                          // ② 未排除
check(active.lastError.isBlank()) { active.lastError }                // ③ 已排除（lastError 为空）
```

④ 之后的写证据、读 json、schema 校验都还没走到。

所以只剩 ① `awaitClosed(15_000)` 超时，或 ② 两次 stop 的完成通知 1 秒内没到齐。

### 已核实 stop 的通知不会丢

```kotlin
val completed = synchronized(closeLock) {
    if (closeComplete) true else { closeListeners += afterClose; false }
}
```
第二次调用时 `closeComplete` 仍为 false → listener **已加入** → 返回 false →
`closing.compareAndSet(false, true)` 失败 → return。listener 仍在列表里，完成时会收到通知。
所以 ① 更可疑：**8 分钟会话关闭超过 15 秒**。

### 结论（诚实版）

- 播放：三次 8 分钟稳态全部 PASS（0.9981 / 0.9983 / 0.9985）。
- 退出：**关闭耗时或通知时延超出测试阈值**，是退出性能问题，不是证据写入问题。
- 我前两轮把方向指向 `writeSyncStats` 是错的，那两处改动（releaseNative 原子化、
  lastError 上报）本身有价值，但没有解决 teardown——**不许把它们记成"已修"**。

### 下一步

在 `stop()` 起点与 `closed.countDown()` 处各取一次时间戳并写进 `lastError`/证据，
直接量出关闭耗时；若确认 >15 s，再看 `native.close()` 里哪一步慢（8 分钟会话的
pending 帧和连接数量远大于 20 秒会话）。

## 21:20 计时结果：teardown 不是慢，是状态残留

```
CHECK teardown stop_ms=0 close_wait_ms=426 closed=true notify_wait_ms=0 notify=true
```

| 断言 | 结果 |
| --- | --- |
| `awaitClosed(15_000)` | **true**，实际只用 **426 ms** |
| `closeNotifications.await(1s)` | **true**，两次通知到齐 |
| `lastError.isBlank()` | **false**（唯一失败项） |

**又纠正一次**：上一轮猜"8 分钟会话关闭超过 15 秒"也是错的，426 ms 就关完了。

### 所以失败的是第三项：`lastError` 非空

`check(active.lastError.isBlank()) { active.lastError }` 抛出 `IllegalStateException`。
但测试的 catch 里 `player_reason=` **没有输出**，说明取到该字段时它已经是空串
（或含被过滤的敏感片段）——两次读取之间被清掉了。

最可能的来源：`host.finish()` → `stop()` 的 `cleanupStep("取消网络读取"){ source?.cancel() }`
把 `HttpRangeFile.failureReason` 置为 `"播放已取消"`；若 `poll()` 在其后还跑了最后一轮，
`if (!summary.contains("error=-")) throw PlaybackFailure(...)` 或
`source?.failureReason?.takeIf{...}` 就会把它写成 `lastError`。

### 已连续猜错两次，记下来

1. 猜 `writeSyncStats` 返回非 0 → **错**（返回 0）。
2. 猜 8 分钟关闭超 15 s → **错**（426 ms）。

两次都源于"看到异常类型就推断原因"，没有先量。教训：**teardown 这种多断言段落，
必须先把它拆开各自计时/取值，再谈原因**——这也是这次能一次定性的原因。

### 下一步（已收窄到"谁写的 lastError"）

在 `stop()` 起点读一次 `lastError` 并记进 report，就能确定它是在
`finish()` 之前就有、还是关闭过程中被写。若确认为 `"播放已取消"` 被当成错误，
正确修法是：**关闭期间由我们主动 cancel 造成的读取失败，不应记为用户可见错误**——
但要先看到那句话，不能凭猜改判定。

## 21:40 最终定位：`clock lead exceeds bound`

放行 teardown 段的断言消息后，异常自己说出了原因：

```
FAIL stage=teardown type=IllegalStateException state=CLOSED
msg=clock lead exceeds bound
```

### 与实测数据完全自洽

| 来源 | 数值 |
| --- | --- |
| 446 s 会话 `clock_lead_ns` | **−827,717,069**（−827 ms） |
| 8 分钟会话整体速率 | 0.9982–0.9988 |
| 推算 | (1 − 0.998) × 480 s ≈ **0.96 s** |

**音频主时钟比墙钟慢约 0.2%，累积到 8 分钟就是 800 ms 量级**，越过测试的
`clock lead` 界。这是音频硬件时钟的固有偏差，不是掉帧、不是卡顿。

### 这解释了为什么 20 秒轮次从来不失败

0.2% × 20 s = 40 ms，落在界内；0.2% × 480 s = 960 ms，必然越界。
**同一个现象，窗口长度决定它是否可见。**

### 三次误判的完整链条（全部留档，别再走）

1. `writeSyncStats` 返回非 0 → **错**，返回 0（`player_reason` 为空自证）。
2. 8 分钟关闭超过 15 s → **错**，实测 **426–444 ms**。
3. `lastError` 残留 → **错**，三个时刻 `err_pre/err_fin/err_final` 全为空。

三次都是"看到异常类型就推断原因"。真正定性的那一步是**放行 teardown 的断言消息**
（它只含帧账/时钟数字，不含地址账号），一句话就把范围关死了。

### 这是不是缺陷：两种情况要分开

- **音频跟随时钟**：`clock_source=audio_timestamp`，视频跟音频，**音画同步正确**，
  整条流比墙钟慢 0.2%——这在任何播放器上都是正常物理现象。
- **断言假设**：测试的 `clock lead` 界隐含"媒体时间应等于墙钟"，对 8 分钟窗口不成立。

但**不能简单判成"测试太严"**：800 ms 的累积领先量意味着若按墙钟做任何调度
（如定时器、外部同步），会真实偏移。项目已有 `drift_gate` / `drift_rebase`，
8 分钟会话的 `drift_rebases` 是多少，是下一步要看的第一个数——
如果 rebase 一次都没发生，那正是"有界纠偏没生效"。

### 下一步（一个数就能继续）

拉 8 分钟会话的 `drift_rebase_trace.rebases` 与 `drift_gate.limit_ns`：
- `rebases > 0` 但仍超界 → 纠偏幅度不足；
- `rebases == 0` → 纠偏根本没触发，问题在门限或触发条件。

## 21:50 根因：`surface_at_time` 下 drift 采样根本没跑

```json
"drift_rebase_trace": {"rebases": 0, "sampled": 0, "max": {"amount_ns": 0}},
"drift_gate": {"enabled": true, "limit_ns": 45000000, "band_ns": 5000000},
"av_drift": {"lead_ns": -645957004, "sync_verdict": "unknown"}
```

| 读数 | 含义 |
| --- | --- |
| `sampled = 0` | **漂移采样一次都没发生** |
| `rebases = 0` | 纠偏从未触发（因为没样本） |
| `gate.limit_ns = 45 ms` | 门是开着的，但没人往里喂数据 |
| `lead_ns = −646 ms` | 未经纠正的累积量 |

### 为什么 sampled 是 0

之前同一份报告里已经有解释：

> "no present-timestamp samples: in surface_at_time the compositor decides when
> a frame is shown, so only the submit lead (schedule_lead_ns) is observable"

**`surface_at_time` 呈现模式下，帧的呈现时刻由合成器决定，我们拿不到 present 时间戳**。
而 drift 的采样与 rebase 依赖 present 样本 → 采样为零 → 纠偏为零 → `clock_lead`
从 0 线性累积到 −646 ms。

### 所以这不是"退出慢"，是"长会话缺少有界纠偏"

- 播放本身正确：音画同步靠 `audio_timestamp`，视频跟音频，**没有不同步**；
- 但 **`clock_lead` 在 `surface_at_time` 下无观测、无纠正**，窗口越长偏得越多；
- 20 秒窗口只有 ~40 ms，看不出；8 分钟必然到 600–800 ms。

### 修法方向（需要设计决定，不是改一行）

用**提交时刻**而不是呈现时刻来驱动纠偏：`schedule_lead_ns` 是可观测的（本会话有样本），
把 drift 的锚点从 present 改成 submit，或直接用 `audio_position − wall_clock` 计算 lead
并在超 `band_ns` 时 rebase。这属于 P13 drift 体系的口径问题，应当由那张卡统一处理，
**不该在播放卡里顺手改**——否则两套口径会打架。

### 这条线的终点

从"点开影片停在正在准备"到现在：

| 层 | 结论 |
| --- | --- |
| 起播 | ✅ 双机 4K HEVC 真实网络 MKV |
| 解码 | ✅ 硬解，24 fps，`media_rate` 0.998–0.999 |
| 稳态 | ✅ 8 分钟 ×4 全 PASS（rolling-2s 未触发） |
| 操作 | ✅ 暂停/恢复/前后 seek，落点为真实关键帧 |
| 长会话 clock lead | ❌ `surface_at_time` 下无采样无纠偏，线性累积 |
| 退出 | 由上一项导致断言失败，非关闭慢、非写证据失败 |

## 22:00 预测被证伪：漂移率不是恒定的

我据 `bound = 45ms + 41.7ms ≈ 86.7ms` 和 ~0.15% 漂移率推算"临界约 58 秒"，
并预测 45 s 通过、90 s 失败。实测：

| 窗口 | 结果 | `rate` |
| --- | --- | --- |
| 45 s | **PASS** | 1.00088 |
| 90 s | **PASS** | 0.99917 |
| 480 s（此前 ×4） | 3 次 FAIL / 1 次 PASS | 0.9982–0.9988 |

**90 秒没有按预测失败。** 原因：这两次的 `rate` 分别是 1.0009 和 0.9992，
漂移率只有 0.01%–0.08%，86.7 ms 的界在 90 秒内累积不到。

所以正确的表述不是"超过 N 秒必挂"，而是：

> **是否失败 = 漂移率 × 窗口时长。** 漂移率在 0–0.2% 之间随机波动，
> 低漂移的会话即使 8 分钟也可能过；高漂移的会话一分钟就超。
> 8 分钟那四轮里三次 `rate≈0.998`，属于高漂移区间。

### 这是第四次预测失误，一起记下来

1. `writeSyncStats` 返回非 0 —— 错。
2. 关闭超 15 秒 —— 错（426 ms）。
3. `lastError` 残留 —— 错（三时刻全空）。
4. 临界 58 秒、90 秒必挂 —— **错**（90 秒 PASS）。

前三次错在"看到异常类型就推原因"，第四次错在"把一个统计量当成恒定值"。
`rate=0.998` 是那几轮会话的观测值，不是这套系统的常数。**用单次会话的速率
去推算另一轮的临界点，从方法上就不成立。**

### 对结论的影响（不变）

- 播放可用：双机起播、硬解 24 fps、暂停/seek、8 分钟稳态速率 0.998–0.999。
- `clock_lead` 在 `surface_at_time` 下 `sampled=0`、`rebases=0`，**无观测无纠偏**，
  这一条是确定的（多份报告一致）。
- 测试用的是**绝对界**（86.7 ms），而漂移是**累积量**；
  两者的张力在长窗口高漂移时才暴露。这是口径问题，归属 P13 drift 体系。

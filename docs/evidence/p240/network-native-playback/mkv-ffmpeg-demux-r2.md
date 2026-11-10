# K40 真实 MKV · 音视频接通与解码速率（2026-11-09 17:05）

前置：[r1 起播](./mkv-ffmpeg-demux-r1.md)、[fd 语义诊断](./fd-semantics-r1.md)

## 结论分两半，必须分开读

**已达成（有证据）：** 片源被识别、硬解打开、音轨接通、暂停/恢复/前后 seek 全部通过。
**未达成：** 视频解码只有 0.67 fps（10 秒内出 2 帧），稳态速率 0.36，远不是可观看状态。
**"能起播"不等于"能看"，本卡不算完成。**

## 通过项（instrumentation 原始行）

```
CHECK production has_audio=true video_codec=OMX.qcom.video.decoder.hevc
PASS production network host starts native hardware video and attaches audio
PASS pause freezes media position and resume advances it
PASS seek request=2 target_us=8000000 landing_us=0 advances_after_completion
PASS seek request=3 target_us=2000000 landing_us=0 advances_after_completion
PASS forward and backward seek use the native A/V session
FAIL stage=steady type=Failure state=PLAYING
PROGRESS gate=rolling-2s rate=0.3645047476261869
```

## 设备端同步报告（`files/evidence/network-native-sync.json`）

```json
"codec": {"video": "OMX.qcom.video.decoder.hevc", "audio": "ffmpeg:eac3"},
"source": {"duration_us": 8829344000,
           "video": {"mime": "video/hevc", "width": 3840, "height": 2076},
           "audio": {"mime": "audio/eac3", "sample_rate": 48000, "channel_count": 2}},
"clock_source": "audio_timestamp",
"frames_received": 2, "frames_released": 2, "frames_rendered": 2,
"decode_fps": 0.665938,
"pace": {"frames_decided": 5, "clock_lead_ns": -14897},
"feed": {"packets_submitted": 266, "frames_decoded": 408576, "frames_dropped": 3072,
         "ring_full_waits": 8874},
"output": {"callbacks": 2416, "frames_rendered": 436608, "frames_silenced": 64896,
           "xrun_count": 1}
```

读法：

| 层 | 状态 |
| --- | --- |
| 容器 | 通过。3 轨识别，4K HEVC + 2ch E-AC3，时长 8829 s |
| 音频 | **正常**。266 包 → 408576 帧（约 8.5 s @48 kHz），主时钟为 `audio_timestamp` |
| 视频 | **不正常**。收到 2 帧、渲染 2 帧、`decode_fps = 0.67` |
| 节拍 | 只决策 5 帧，`clock_lead = -14.9 µs`（没有明显的对齐偏移） |

音频链路健康，说明"网络读 → demux → 解码 → AAudio"这条链本身是通的。
问题被隔离在**视频解码吞吐**这一处，不是网络、不是容器、不是时钟。

## 当前最可能的三个原因（都还未验证，不许当结论用）

1. **csd-0 形状**：现在把 MKV 的 `CodecPrivate`（hvcC）原样透传。若解码器把参数集
   当成每帧都要重解析，表现正是"能解但极慢"。判据：改成 AnnexB 参数集后比
   `decode_fps`，而不是比"能不能播"。
2. **4K 解码能力**：3840×2076 超出常见档位，K40（SM8250）标称支持 4K60 HEVC，
   但要看实际配置的 profile/level/码率。判据：同一 demux 换 1080p HEVC 源做对照。
3. **供包粒度**：`AvioBridge` 32 KiB 一次 `pread`，4K 关键帧可能几百 KiB，
   叠加共享 `ioLock` 与音频 feeder 竞争。判据：看视频供包线程的等待时长分布。

排序理由：1 和 3 是我这次改动引入的变量，2 是片源属性；先排自己引入的。

## 不许做的"修复"

- 把 `decode_fps` 低解释为"4K 就这样"——没有 1080p 对照之前，这只是猜测。
- 降低稳态门限或缩短采样窗口让它"通过"。
- 打开软解视频兜底（明确红线）。

## 复现

```bash
adb -s b6e950ee shell am instrument -w -e realSource 1 -e realItem 2 -e realPreflight false \
  com.kivo.cinema.test/com.kivo.cinema.playback.NetworkAudioConnectionInstrumentation
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/evidence/network-native-sync.json"
```

## 边界

- 未测：首帧画面是否正确（无截图）、音画同步精度、30 s 以上连续播放、平板回归。
- `landing_us=0` 的 seek 落点读数可疑（目标 8 s 却报 0），虽断言 `advances_after_completion`
  通过，仍需单独核对——落点为 0 会让"是否真跳到 8 s"永远无法证伪。

## 附：一次被证伪的假设（17:12）

假设：视频只出 2 帧是 AVIO 缓冲太小（32 KiB 一次 `pread` = 一次网络往返，128 KiB 实测
13–20 ms），把缓冲放大到 1 MiB 应能提升吞吐。

做法：`kIoBufferSize` 32 KiB → 1 MiB（与响应环同尺寸），重编、重装、重跑。

结果：**证伪。** `decode_fps` 0.666 → 0.666，`frames_received` 仍是 2。

```
PROGRESS gate=rolling-2s rate=0.36233333333333334
"frames_received": 2, "frames_rendered": 2, "decode_fps": 0.666185
```

结论：供包吞吐不是瓶颈。缓冲放大保留（它确实减少往返，不构成退化），但**不能解释**
这个问题，不许把它记成"修复"。

## 真正指向的地方

同步报告里这条是决定性的：

```json
"assessment": {
  "total_us": 923773,
  "pass": false,
  "reason": "first frame pts 14083000 us is 14083000 us after target 0 us
             (extractor landed at 0 us) — the decoder started at a later keyframe,
             so this is a landing / keyframe-spacing problem, not decode latency"
}
```

**第一帧 pts 是 14.083 s，而 seek 声称落点是 0。** 播放只跑了约 10 秒，从 14 秒处起解，
自然只能出 2 帧。`decode_fps` 是"2 帧 ÷ 10 秒"的结果，不是解码慢。

所以问题不是解码吞吐、不是网络、不是 csd，而是 **seek 落点与 pts 读数**：
`seek_to()` 返回 0 与真实首包 pts 14.083 s 互相矛盾，两者必有一个在说谎。

下一轮只需回答一个问题：`av_seek_frame` 之后读到的第一包 pts 到底是多少，
以及 MKV 的 `AVStream.start_time` / `time_base` 是什么。在这之前不再改其他层。


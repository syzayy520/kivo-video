# 「卡成 PPT」的实测定位（2026-11-09 17:35）

用户现场反馈：画面卡成 PPT。本卡用 20 秒稳态测试把它拆成数字，不再靠描述。

## 一句话结论

**不是网络卡、不是容器慢、不是 seek 错——是视频解码 10 秒只出 2 帧，
而 demux 已经读了 10.3 MB 数据。** 数据够，解码不出。

## 关键读数（`-e steadyOnly true`，20 秒稳态）

```
CHECK production has_audio=true video_codec=OMX.qcom.video.decoder.hevc
FAIL stage=steady-without-operations
PROGRESS gate=rolling-2s rate=0.3744
media_curve: 0:0.0, 1004:0.0, 2009:1.766761, 3013:1.766761
```

```json
"frames_received": 2, "frames_rendered": 2, "frames_decided": 2,
"decode_fps": 0.63785, "media_rate": 1.000032763,
"output": {"callbacks": 935, "frames_rendered": 168960, "xrun_count": 0, "wall_millis": 3612}
```

| 事实 | 含义 |
| --- | --- |
| 位置推到 **1.766 s 后不动** | 不是"慢"，是停住 |
| `frames_received = 2` | 解码器 10 秒只交出 2 帧 |
| `decode_fps = 0.64` | 平均 1.5 秒一帧 |
| 音频 `callbacks=935`、`xrun=0`、`silence=0` | 音频链路健康，有声音 |
| `media_rate = 1.00003` | 时钟没有被拖慢的迹象 |

## 排除项（都有数据）

- **网络**：reader=2（视频）持续读到 `offset=10305536`，10 秒 524 条 IO 事件，
  单次 128 KiB 耗时 13–20 ms。数据已经到位。
- **容器**：3 轨识别、4K HEVC、时长 8829 s、csd 存在。
- **seek**：前后 seek 断言 `advances_after_completion` 通过。
- **AVIO 缓冲**：32 KiB → 1 MiB 后 `decode_fps` 0.666 → 0.666，证伪。
- **起点复位**：`find_stream_info` 后 `av_seek_frame(start_time)` + flush 已加，
  `first frame pts` 仍为 14083000，未改变结论。

## 剩下的两个候选（还没验证，不许当结论）

1. **csd 与样本封装不一致**：现在把 MKV 的 `CodecPrivate`（hvcC）原样给 `csd-0`，
   样本是 AVCC 长度前缀。若 MediaCodec 对这个组合退化成慢路径，
   表现正是"能解但极慢"。
2. **4K 解码能力**：3840×2076 在这台 SM8250 上究竟是硬解还是走了低效模式。
   需要 **1080p HEVC 同源对照**才能分清——没有对照就说"4K 就这样"是猜。

判定顺序：先做 1080p 对照（成本低、能一刀切开"能力问题"与"配置问题"），
再决定是否动 csd。

## 17:58 追加：解封装已被排除，问题钉在解码器配置

从同一份 sync 报告里取到一个此前没注意的字段：

```json
"samples_submitted": 914,
"samples_rejected": 0,
"frames_received": 2,
"schedule_lead_ns": {"samples": 2, "p50": -119875000}
```

**914 个视频样本提交给解码器，0 个被拒，却只输出 2 帧。**

这一条把候选 1 之外的猜测全部关掉：

| 层 | 结论 |
| --- | --- |
| 网络 | 已排除（reader 读 10.3 MB，13–20 ms/次） |
| 容器解析 | 已排除（3 轨、csd 在、时长正确） |
| 解封装供包 | **已排除**（914 包已提交，无一被拒） |
| 解码输出 | **问题在这里**：914 进 → 2 出 |

914 包 / 约 40 秒 ≈ 23 包/秒，与 4K 24 fps 片源吻合；供包节奏是正常的。
`schedule_lead p50 = -119.9 ms`（负值）说明提交长期落后于呈现时刻，
与"解码器几乎不出帧"一致。

所以下一步的唯一动作是**校验交出去的 csd 形状**：
打印 `extradata` 前 32 字节，判断它是 hvcC（`01` 开头 + `numOfArrays`）还是 AnnexB
（`00 00 00 01` 开头），再看样本首字节是长度前缀还是起始码。
**只有这两者匹配关系确定之后，才允许改 `csd-0`。**

1080p 对照这条路本次没走通：本地测试固定读 `media/local-audio-connection.mp4`，
把它换成 4K 样本后测试在第一个断言就失败（该测试依赖有音轨的合成片），
换回原文件并已恢复。网络侧 1080p 片源尚未确认是哪一条 item。


## 不许做的

- 降低稳态门限、缩短采样窗口让它"通过"。
- 用软解兜底（红线）。
- 宣布"能起播"等于"修好了"——现在有声音、有 2 帧、然后停住，不能看。

## 18:10 追加：csd 假设被 A/B 证伪，1080p 对照源不可用

**A/B 实验**：把 hvcC 拆成 AnnexB 参数集写入 `csd-0`（样本仍为 AVCC），重编重装重跑：

| 指标 | hvcC 透传（原） | AnnexB csd |
| --- | --- | --- |
| `frames_received` | 2 | **3** |
| `rate` | 0.36 | **0.0** |
| `samples_submitted` | 914 | 1193 |

**更差。** csd 形状不是根因，hvcC + AVCC 的原始组合至少还能出帧。转换代码已删除，
不留无用分支。

**1080p 对照失败**：换 `realItem 3` 得到 1920×1080 H.264（走**平台**路径，不经 FFmpeg），
结果 `frames_received=0`、`samples_submitted=1910`、`decode_fps=0`，
在 `stage=tracks` 的 `position > 1s` 断言上超时。

这条**不能当 4K 对照**：它自己就 0 帧，最可能是该片源的编码档位（如 10bit H.264）
本机硬解不支持，且项目禁止软解回退。用一条自己都不出帧的片源去证明"4K 是能力问题"
是无效推理。

对照基线（改动前 r16，同一台 K40）：
`640×360 H.264`、`frames_received=541`、`decode_fps=30.26`、codec `OMX.qcom.video.decoder.avc`。
说明平台路径**曾经**正常，但那是 360p。

## 当前唯一未解的问题

**914 个视频包提交、0 拒绝、只出 2 帧**，且 csd 形状已被排除。
下一步要一条**确认能硬解的 1080p HEVC 合成片**（不是网络片源、不是 10bit H.264），
在同一 demux 下比 `frames_received`。
在那之前不能碰解码器配置——没有可用对照，任何改动都无法归因。

## 18:58 现场：K40 在本地 4K 起播后掉线（原因未确认）

按方案 2 用 `kivo.media` 直接播设备上的 `a211_4k.mp4`（平台路径，不经 FFmpeg demux）：

```
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  -e kivo.media /sdcard/Android/data/com.kivo.cinema/files/media/a211_4k.mp4 -e kivo.label test4k
Starting: Intent { cmp=...PlayerComposeActivity (has extras) }
```

起播 6 秒后第一次 `screencap` 得到 303 KB，3 秒后第二次为 **0 字节**；
随后 `adb devices` 显示 **无设备**，`adb reconnect` 与 18 秒轮询均未恢复。

**不能据此断言"4K 播放导致崩溃"**：也可能是息屏/USB 松动/系统重启。
需要设备恢复后查看 `ro.boot.bootreason` 或 `dumpsys activity` 里的进程存活时间才能定性。
但这确实发生在 4K 本地起播之后，作为现象必须留档，不许在复盘时被漏掉。

**设备恢复后第一件事**（顺序不能变）：
1. `adb shell getprop ro.boot.bootreason`、`adb shell uptime` 判断是否重启过；
2. 若未重启 → 4K 解码把系统拖死，这比"0.67 fps"严重得多，优先级要提到最高；
3. 若重启过 → 拉 `last_kmsg`/`dmesg` 看是否 GPU/解码器崩溃；
4. 一切正常后才继续 1080p HEVC 对照。

## 19:05 离线排查：配置链无问题，改掉一个时间戳隐患（待验证）

设备仍离线，转做不依赖设备的代码审查：

- `make_configure_format_with_mime`（`mediaformat_color_io.cxx:74`）**显式拷贝 `csd-0`/`csd-1`**，
  注释还写明"没有 SPS/PPS 连 configure 都过不去"。csd 不会在这一层丢。
- `decode_session.cxx:65` 的格式校验失败返回 `kFormatRejected`，不会静默继续。
  会话能进 `PLAYING` 说明校验已通过。

**发现并修掉一处**：`ffmpeg_demux_source.cxx` 里两处
`pts = (packet_->pts == AV_NOPTS_VALUE) ? 0 : packet_->pts`。
直接给 0 的后果不是"差一点"——所有缺 pts 的包变成同一时刻，pacer 看到重复
时间戳只会决策极少数帧，表现正是"914 个包进去、2 帧出来"。平台 extractor 保证
包必有时间戳，libavformat 不保证（B 帧重排后个别包 `pts` 缺失而 `dts` 有效）。

已改为 `packet_time()`：pts → dts → 0 逐级兜底，两处调用点统一。

**这是推理得出的修复，不是已验证的修复。** 设备恢复后必须重跑：
判据只有一条 —— `frames_decided` 是否从 2 明显上升。
若仍是 2，说明时间戳不是根因，不许把它记成"已修"。

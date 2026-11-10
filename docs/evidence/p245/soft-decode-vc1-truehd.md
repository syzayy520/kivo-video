# 软解（VC-1 + TrueHD）SMB 播放验收记录

日期：2026-11-10　设备：平板 MRR-W29（kona / SM8250，8 核，大核 2.42 GHz，超大核 3.19 GHz）
片源：`\\192.168.5.1\视频\电影\[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT\…mkv`
（20.17 GB / 1920×1080 / VC-1 AP / TrueHD 5.1 48 kHz 6ch / matroska）

## 一、结论先说

| 项 | 结果 |
|---|---|
| 视频软解（VC-1） | **能播**：解码、上屏、seek、暂停/恢复全部通过 |
| 音频软解（TrueHD） | **能出声**：demux → truehd 解码 → swresample → AAudio 全链路打通 |
| 播放速率 | **0.31x 实时**（10 秒墙钟只走 3.09 秒媒体），约 11 fps |
| 瓶颈 | 解码：88–90 ms/帧，**单个大核吃满 94%**，无法并行 |
| 可否修好 | **不能**。FFmpeg 的 VC-1 解码器没有任何线程能力，且设备无 VC-1 硬解 |

## 二、验收用例（`SoftDecodeSmbInstrumentation`，15/15 PASS）

```
[THROUGHPUT] read=24 MiB in 2.02s = 11.90 MB/s
[ROUTE]      decision=ffmpeg useFfmpeg=true reason=no MediaCodec decoder for video/vc1
[PASS] 路由判定为软解（VC-1 无平台解码器）
[PASS] 软解打开（demux + 解码器）
[PASS] 起解线程
[PASS] demux 初始化（20 秒内探到时长）
[PASS] 位置推进（3 秒内前进 > 1 秒）
[PASS] 暂停后位置冻结
[PASS] 暂停状态下 seek 到中点      → target=2802821500 landing=2802758000 diff=-63 ms
[PASS] 恢复后继续推进
[PASS] 持续播放 10 秒的实际速率    → media=3.09s wall=10.00s ratio=0.31x
[PASS] 音频链路起得来
[PASS] 音频 20 秒内真正出声（AAudio 流已开）
[PASS] 音频帧持续写入（不是开流后卡住）
[PASS] 音频暂停后位置冻结
[PASS] 音频恢复后继续
[RESULT] 15/15
```

音频证据：

```json
{"codec":"truehd","container":"matroska,webm","packets_read":3147,
 "frames_written":123320,"ring_full_waits":294,"read_errors":0,
 "src_rate":48000,"src_channels":6,"out_rate":48000,"eos":false,"error":""}
```

视频证据（`kivo.p211.decode/1`）：

```json
"counters": {"packets_read":1724,"video_packets":172,"keyframes":172,
             "frames_decoded":170,"frames_rendered":170,
             "render_failures":0,"read_errors":0},
"timing":   {"wall_us":30290107,"read_ms_total":5894.8,
             "decode_ms_total":15304.7,"render_ms_total":362.2},
"source":   {"width":1920,"height":1080},
"pts_diag": {"first_packet_pts":0,"first_media_us":42000,"frames_without_pts":1}
```

## 三、时间到底花在哪（这是本轮最重要的取证）

| 项 | 数值 | 说明 |
|---|---|---|
| 每视频帧解码 | **≈ 88 ms** | `decode_ms_total / frames_decoded`，纯 CPU，单核 |
| 每视频帧读包 | ≈ 34 ms | `read_ms_total / video_packets`，与解码串行 |
| 每视频帧渲染 | ≈ 2 ms | `render_ms_total / frames_rendered` |
| 合计 | ≈ 124 ms/帧 | 24 fps 需要 41.7 ms → 差 3 倍 |

排除掉的怀疑对象（都实测过，不是瓶颈）：

- **网络**：PC 从同一共享读 94.8 MB/s；平板自己读 11.9 MB/s；播放实际只用到约 1.2 MB/s。有 8 倍余量。
- **测量工具**：最初用 `SurfaceTexture.updateTexImage()` 当消费端，1080p 每次要 ~20 ms 的 GL 拷贝，把一整核吃满（96% CPU / 17.9 s CPU 时间）。换成 `ImageReader` 出队即释放后，速率**没有变化** → 工具确实不是瓶颈，但读数现在是干净的。
- **调度**：进程在 `cpuset:/foreground`、`schedtune:/foreground`，`Cpus_allowed_list: 0-7`，没有降级。解码线程实际落在 **cpu4（2.42 GHz A77 大核）**，占用 94.4%。
- **SIMD**：`libavcodec/aarch64/vc1dsp_neon.o` 与 `vc1dsp_init_aarch64.o` 均在产物里，NEON 汇编已启用。
- **swscale**：试过 `SWS_POINT`，速度无变化（±3%），且 4:2:0 色度会变最近邻导致彩边锯齿 → 已回退 `SWS_BILINEAR`。

## 四、为什么修不好

```
$ grep -n ".capabilities" libavcodec/vc1dec.c
1406:    .p.capabilities = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_DELAY,
1445:    .p.capabilities = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_DELAY,
1484:    .p.capabilities = AV_CODEC_CAP_DR1,
1499:    .p.capabilities = AV_CODEC_CAP_DR1,
```

`AV_CODEC_CAP_FRAME_THREADS` / `AV_CODEC_CAP_SLICE_THREADS` **一个都没有**。
VC-1 在 FFmpeg 里是严格单线程解码器，`thread_count` 设成 8 也只会有 1 个线程干活。
所以 1080p VC-1 在这台设备上的上限就是「一个大核的 1/88ms」≈ 11 fps。

设备也没有替代路径：

```
$ cat /vendor/etc/media_codecs*.xml | grep -o 'video/[a-z0-9._-]*' | sort -u
video/3gpp  video/av01  video/avc  video/hevc
video/mp4v-es  video/mpeg2  video/x-vnd.on2.vp8  video/x-vnd.on2.vp9
```

没有 vc1/wmv。VLC / Kodi / MX Player 用的是同一份 FFmpeg 的同一个 `vc1` 解码器，
在这台设备上同样只能是这个速度；要 1080p VC-1 实时，只能靠硬件 VC-1 解码器
（本机没有）或更快的 CPU。

## 五、本轮实际改动

1. **AVIO 标可 seek**（`ffmpeg_custom_io.cpp`）
   `avio->seekable = AVIO_SEEKABLE_NORMAL`。不标的话 mkv 读不到 Cues，时间戳不可靠。
2. **`pts=N/A` 时回退 dts**（`ffmpeg_adapter.cpp` / `read_packet`）
   Matroska 的 VC-1 包只有 dts，原代码把 `AV_NOPTS_VALUE` 映射成 0 → 整轨时间戳归零。
3. **`av_seek_frame` + `AVSEEK_FLAG_BACKWARD`**（`seek_file_us`）
   `avformat_seek_file` 传 `INT64_MIN/MAX` 边界会被 matroska 拒绝。
4. **只保留需要的轨道**（`discard_streams_except`）
   `packets_read` 从 8154 降到 1724；语义上也不再为丢掉的包分配内存。
5. **修 `swr_convert` 的参数错误**（`soft_audio_session.cxx`）
   原来写的是 `(uint8_t**)converted.data()`，等于让 swr 把 PCM 缓冲区头几个字节当平面指针数组，
   真机崩溃在 `swr_convert → swri_audio_convert`，`fault addr 0x4`。改成显式 `out_planes[1]`。
6. **音频暂停要停 AAudio 流**（`soft_audio_session.cxx`）
   只置标志的话环里 500 ms 还会继续响 —— 用户按了暂停声音还在走。
7. **音频 seek 要停流 + 清环**（`soft_audio_session.cxx`）
   不清环时定位后头 500 ms 放的是定位前的声音。
8. **新增 `read_ms_total`**（`ffmpeg_local_playback_session.*`）
   把「读」与「解」分开记账，否则下次还得靠猜。

## 六、复现命令

```bash
export MSYS_NO_PATHCONV=1
ADB="C:/Android/sdk/platform-tools/adb.exe"; D=CFE6R21B12000179
cd "C:/kivo video/android"
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"

./gradlew :app:assembleDebug :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SoftDecodeSmbInstrumentation -q
$ADB -s $D install -r -t app/build/outputs/apk/debug/app-debug.apk
$ADB -s $D install -r -t app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk

$ADB -s $D shell am instrument -w \
  -e smbHost 192.168.5.1 -e smbShare 视频 \
  -e smbPath "电影/[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT/[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT.mkv" \
  com.kivo.cinema.test/com.kivo.cinema.smb.SoftDecodeSmbInstrumentation
```

注意：测试包必须带 `-Pkivo.testRunner=…`，否则会打成 `FilmMarksInstrumentation`，
`am instrument` 会报 `Unable to find instrumentation info`。

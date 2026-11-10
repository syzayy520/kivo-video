# P249 — 10bit HEVC 误判软解 + TrueHD 音轨接入 + SMB 多轨缓存

日期：2026-11-10
设备：MatePad MRR-W29（SD865 / kona，Android 12，2560×1600@60Hz）
片源：`Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv`
　　　HEVC Main10 / 1920×804 / 23.976fps / 12.2 GB / 2×TrueHD 5.1 + DTS 768k + PGS

## 一、用户报的现象

> 「恐怖废墟能看你了，shelter 这部片也卡成 PPT」

恐怖废墟是 VC-1（设备无解码器，走软解），上一轮已修到 24.04 fps。
Shelter 换成 HEVC Main10，同样走软解，但软解跑不动。

## 二、根因一：路由把「平台表里没写这个 profile」当成「平台没有解码器」

起播日志：

```
KivoCompose: smb routing: ffmpeg / no MediaCodec decoder for video/hevc
```

`FfmpegPlaybackSession.decide()` 用 `MediaCodecList.findDecoderForFormat(format)` 判定。
这个方法会拿 `MediaFormat` 里的 profile/level 去**严格比对设备声明的 profile 表**，
表里没列到就返回 null。

本机 `/vendor/etc/media_codecs*.xml`：

```
<MediaCodec name="c2.qti.hevc.decoder" type="video/hevc">
    <Limit name="size" min="128x128" max="4096x2304" />
    <Limit name="frame-rate" range="1-480" />
    <Limit name="performance-point-1920x1080" value="240" />
    <Limit name="performance-point-3840x2160" value="60" />
    ...
```

没有 `<ProfileLevels>` 段（grep `HEVCProfile|Main10` 无结果）。于是 Main10 被判成
「无解码器」，整片退到 FFmpeg 软解。

**反证（不依赖任何代码）**：把一个 4 秒 1920×804 / yuv420p10le / main10 的样片推到设备，
用系统播放器打开，日志直接给出：

```
MediaCodecList: matching 'OMX.qcom.video.decoder.hevc'
OMX-VDEC-1080P: component_init: OMX.qcom.video.decoder.hevc
ACodec: In onAllocateComponent create component, codec name: OMX.qcom.video.decoder.hevc
MediaCodec start
```

系统播放器只按 mime 匹配（`findMatchingCodecs`），所以能硬解同一个文件。

软解代价（实测）：

```
frame_budget frames=240 fps=20.1 per=49.7ms | send=0.4 sws=20.5 resize=0.4 read=0.7 render=4.0
```

`sws=20.5 ms` 是 10bit → 8bit + 缩放 + YUV→RGBA 的固有成本，`decode_total=21.4ms`，
单帧总成本超过 23.976fps 的 41.7ms 预算，稳定掉帧约 20%。

## 三、根因二：硬解路径的音频不认 TrueHD

`AudioPlayback::open()` 里本来就有"平台没解码器就走 FFmpeg"的分支，
但条件写死成 `audio/ac3 || audio/eac3`：

```cpp
if (track.mime == "audio/ac3" || track.mime == "audio/eac3") { ... FfmpegAudioDecoder ... }
```

这部片三条音轨是 `audio/true-hd` ×2 + `audio/vnd.dts`，全在名单外，于是走
`MediaCodecAudioDecoder` → 设备无解码器 → `session audio` 失败 → 再被判回软解。

`FfmpegAudioDecoder::configure()` 同样只映射了 EAC3/AC3 两个 codec_id。

## 四、根因三：硬解路径的 SMB 读缓存命中率为 0

路由修好后先只加了路由，得到：

```
KivoSmbRead: stats reads=27 hits=0 (0%) prefetch=0 fills=27 fill_avg=152.9ms fill_max=238.4ms
```

27 次读、27 次未命中、每次同步读 152 ms。原因是 `SmbByteSource` 只有一个 1 MiB 窗口，
而硬解由 MediaExtractor 驱动，按**轨道**交错读取：读视频块 → 读音频块 → 回视频块，
每次换轨都把上一个窗口整块顶掉。

（软解路径没这个问题：FFmpeg demux 是一个读指针顺序前进，预读命中率 100%。）

## 五、修复

### 5.1 路由：按 mime 找硬件解码器兜底

`FfmpegPlaybackSession.decide()`：`findDecoderForFormat` 返回 null 时，遍历
`MediaCodecList`，按 mime 匹配、**只认硬件**（`isHardwareAccelerated`，API<29 用名字排除
`OMX.google.*` / `c2.android.*`）、再校验 `isSizeSupported(width, height)`。

只认硬件是刻意的：软解解码器（OMX.google.*）能创建不代表能实时播放，
放进来只会换来更差的软解 + 一条没走通的同步链路。

### 5.2 音频：TrueHD / DTS 纳入 FFmpeg 软解

- `audio_playback.cxx`：名单扩到 `ac3 / eac3 / true-hd / vnd.dts`
- `ffmpeg_audio_decoder.cxx`：codec_id 映射加 `AV_CODEC_ID_TRUEHD` / `AV_CODEC_ID_DTS`

PCM 侧本来就是通的：支持 FLT/S16/S32、平面/打包，通道布局接受 mono/stereo/5.1。

### 5.3 SMB：单窗口 → 按块对齐的 6 块 LRU

- `BLOCK_BYTES = 2 MiB`（1 MiB 时命中率只有 82%，交织距离会跨过块边界）
- `BLOCK_COUNT = 6`（视频/音频/PGS 三条轨常驻 + 3 块机动，共 12 MiB）
- 块按 2 MiB **对齐**：不对齐等于每次请求都换一段区间，缓存永远对不上
- miss 后的补齐**不再返回 -1**：跨块边界时退化为 `directRead`，避免被上层当 EOF

命中率：**0% → 87~95%**。

### 5.4 预读：改成按路径显式开关，不再靠启发式

实测 A/B（同一片源、同一台设备）：

| 配置 | 帧率 | 帧间隔 max |
|---|---|---|
| 硬解 + 预读开 | 20.27 fps | 567 ms（末尾连续 183/233/266/567） |
| 硬解 + 预读关 | 24.00 fps | 50 ms |

预读用**第二条 SMB 连接**，和主读分同一条链路带宽。硬解的访问是"突然要一大片"，
预读提前读的数据大半没被用上，却把主读挤慢了。

所以 `SmbByteSource.prefetchEnabled` 默认 **false**，由调用方按路径设置：

- 软解视频源：`setPrefetchEnabled(true)`（顺序流式，命中率实测 100%）
- 软解音频源：`setPrefetchEnabled(true)`（同样是顺序流式；漏设会导致每 1 MiB 一次
  213 ms 同步读，音频环随时被抽干）
- 硬解：不设（保持 false）

## 六、验证

### 6.1 路由与音频

```
KivoCompose: smb routing: mediacodec / hardware decoder 'OMX.qcom.video.decoder.hevc'
             via mime fallback for video/hevc (profile table lacks this profile)
KivoCinema: session open ok: network-media (fd=146 bytes=12186004520)
KivoCinema: session audio ok: network-media (fd=153 bytes=12186004520)
KivoCinema: session start ok
```

`dumpsys audio`：本进程 AAudio `state:started`，48 kHz，1 条 active track。

### 6.2 画面

跳转到片中以避开片头静态 logo 后连续截图，4 秒内差异区域覆盖全屏
（`(0, 80, 2560, 1600)`），画面为海边悬崖 / 红色渔船的实拍镜头，无花屏、无绿块、
色彩正常，左上角「正在播放」且**无「软解」字样**。

### 6.3 资源开销

| | 软解 | 硬解 |
|---|---|---|
| 进程 CPU | 59.3% | 43.5% |
| 系统 iowait | 119% | 75% |

### 6.4 软解路径零回归（恐怖废墟 VC-1）

```
frame_budget frames=360 fps=21.2 per=47.1ms | send=12.1 sws=2.2 read=15.8 render=2.3
KivoSmbRead: stats reads=128 hits=128 (100%) prefetch=128 fills=0
KivoSmbRead: stats reads=132 hits=132 (100%) prefetch=132 fills=0
```

逐窗口帧间隔 41.5 / 41.2 ms → 稳态 24.1 / 24.3 fps，两个源（视频 + 音频）均 100% 命中、
零同步读。

## 七、遗留

1. **3:2 pulldown 抖动**：本机屏幕只有 60.000004 Hz 一个模式
   （`supportedModes` 仅 id=1，`alternativeRefreshRates=[]`），23.976fps 内容必然呈现为
   33/50 ms 交替。这是 60Hz 屏播放 24p 的物理限制，不是播放器缺陷。要消掉需要设备支持
   48/120 Hz 显示模式。
2. **`SurfaceFlinger --latency` 在本机不可用于 SurfaceView**：连续两次采样完全相同
   （`SAME`），无法作为帧率证据。本轮改用逐帧日志（软解）/ 截图差异 + 时间码（硬解）。
3. 硬解路径的 `miss_blk` 在拖动进度条时会出现块号倒退（如 `[90..73] jump_max=23`），
   属 seek 后的正常回退访问，块缓存按 LRU 处理。

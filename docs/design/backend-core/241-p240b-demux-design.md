# P240-B · 容器解析替换（MKV 走 FFmpeg demux）设计

日期：2026-11-09
前置证据：[代理 fd 语义诊断](../../evidence/p240/network-native-playback/fd-semantics-r1.md)、
[播放阶段交接](../../evidence/p240/network-native-playback/software-audio/status.md)

## 一、为什么必须换

同片源（7,369,631,541 字节 MKV）两台设备行为相反，而 fd 能力完全一致：

| 判据 | K40 / Android 13 | 平板 |
| --- | --- | --- |
| `fstat` 长度 | 7369631541 正确 | 正确 |
| `lseek(SEEK_END)` | 返回 7369631541 | 可用 |
| 尾部 `pread` 8192B | 成功，8186 字节非零 | 成功 |
| extractor 是否请求尾部 | **从未请求** | 头部读完直接请求尾部 |
| 结果 | 45 s 后 0 轨 | 7.7 s 识别 3 轨 |

fd 能读尾而平台 demux 不发这个请求，说明差异在**平台 MKV 解析策略**。
改 fd、改缓存、放宽超时都不会让它开始读索引。r1–r16 把网络层优化到零重复建连
仍然过不去，正是因为瓶颈不在这层。

**因此：容器解析改用 FFmpeg `avformat`。** 它自己解析 SeekHead → Cues，不依赖
平台版本。本机 `C:/re-tools/ffmpeg-android/dist/lib/libavformat.a` 已存在，
`kivo_backend_ffmpeg` 已链接 `avformat`，不需要新依赖。

## 二、边界（不许越过的线）

只替换**容器解析**这一件事。以下全部不动，也不允许借这次改动顺手动：

- 解码（`mediacodec/decode`）、音频供数（`avsync/session`、`audio_decode_feeder`）
- 时钟与节拍（`core/pace`、`aaudio/clock`）、呈现与 Surface 生命周期
- 网络读取所有权：`net/HttpRangeFile` 仍是唯一可寻址网络文件职责
- 禁止用 FFmpeg **软件解码视频**兜底。换的是 demux，不是 decoder。

选择结果必须可读：证据与 summary 里写明 `demux=platform` 或 `demux=ffmpeg:matroska`，
不允许"能播就行、不说用了哪条路"。

## 三、自然家族树

```text
backend/include_private/video/video_plane/
  backends/demux/
    sample_source.hpp          # 接口：与现有 MediaExtractorSource 同形
    demux_selection.hpp        # 容器判定与实现选择（纯策略，无 IO）
  backends/mediacodec/source/
    mediaextractor_source.hpp  # 现有实现，改为实现 SampleSource
  backends/ffmpeg/demux/
    ffmpeg_demux_source.hpp    # avformat 实现
    avio_bridge.hpp            # fd ↔ AVIOContext（只做读写定位，不解封装）
    codec_config_builder.hpp   # extradata → AMediaFormat(csd-0/1)
backend/src/video/video_plane/backends/ffmpeg/demux/
  ffmpeg_demux_source.cxx      # 打开、选轨、读包、seek、错误
  avio_bridge.cxx              # seek/read 回调；AVSEEK_SIZE 返回真实长度
  codec_config_builder.cxx     # hvcC/avcC → AnnexB 参数集；mime/宽高/时长
```

职责一句话：`avio_bridge` 只会读写定位；`ffmpeg_demux_source` 只会解封装；
`codec_config_builder` 只做格式翻译。三者互不调用对方的私有状态。

`core/` 不含任何 Android 头，所以接口放在 `backends/`——它必须返回 `AMediaFormat*`，
这是解码层现有的唯一契约，绕开它等于重写 decode_session。

## 四、四个必须写死的契约

### 1. 格式翻译：extradata → csd-0

MediaCodec 要的是 `AMediaFormat`，而 FFmpeg 给的是 AVCodecParameters：

- H.264：`extradata` 是 avcC → 拆出 SPS/PPS，按 4 字节起始码拼成 AnnexB 写 `csd-0`；
- HEVC：`extradata` 是 hvcC → 拆出 VPS/SPS/PPS（数组按 NAL 类型），同样 AnnexB；
- 时长取 `st->duration * time_base`，宽高取 `codecpar->width/height`。

**不解码就不能靠猜**：参数集拆不出来时显式失败（`AMEDIA_ERROR_MALFORMED` 语义），
不许塞空 csd 让解码器自己崩。

### 2. 包格式必须与 csd 配套

FFmpeg 交出的 `AVPacket` 是长度前缀（AVCC）格式，csd 若按 AnnexB 提供，
两者不一致会花屏或直接 `-10000`。**同一实现内 csd 与 sample 必须是同一种封装**，
并在首次成功解码后记录到证据，不能只在代码注释里声明。

> **落地缺口与本条的关系（2026-11-10，244）**：本条要求"csd 与 sample 同一种封装"，
> 而 §四.1 已规定 csd 按 **AnnexB** 提供 —— 因此**每一种编码都必须挂对应的 bitstream filter**
> （H.264 → `h264_mp4toannexb`，HEVC → `hevc_mp4toannexb`）。
> 首版实现只给 HEVC 挂了，H.264 分支漏掉，MediaCodec configure 成功却吞掉全部样本：
> `samples=600 frames=0 speed=37.83x`、位置恒 0、画面全黑（244 卡）。
> **这不是新问题，是本条契约的落地没做全**；244 补齐 `h264_mp4toannexb` 后即回到合规态。
> 教训：契约写成"必须配套"还不够，得**按 `codec_id` 穷举并让 `default` 分支显式失败**，
> 否则漏一种编码就是静默黑屏。

### 3. seek 语义

`av_seek_frame(..., AVSEEK_FLAG_BACKWARD)` → `avformat_flush` → 返回实际落点。
返回的是关键帧 pts，不是请求目标；调用方（现有 `PlaybackSession.seekTo`）已经按
"实际落点"契约工作，不许在这里改成返回请求值。

### 4. 取消与退出

`avformat_find_stream_info` 和读包都会发起网络读，沿用现有取消路径：
宿主先 `HttpRangeFile.cancel()` 断连接，再 native stop/join。AVIO 回调必须在
每次读之前检查取消标志，抛出可识别的错误码，禁止阻塞在 socket 上等超时。

## 五、选择策略（不静默、不重试成瘾）

```text
读头部 ≤ 4KiB → 判容器（EBML 0x1A45DFA3 / ftyp / TS sync）
  matroska|webm → ffmpeg
  其他          → platform（现有已验证路径不动）
```

为什么不一刀切全走 FFmpeg：平板那台在 platform 路径上有 r16c 真机稳态证据，
一次全换等于把已验证的东西全部作废。MKV 是唯一已证失败的容器，先只切它。

为什么不用"先试 platform、失败再回退"：K40 上 platform 失败要 45 秒才返回 0 轨，
回退意味着每次起播先罚 45 秒。选择必须是**打开前**完成的判断。

## 六、Java 侧必须同源

`NetworkPlaybackSession.probe()` 现在自己开一个 `MediaExtractor` 报轨道数。
K40 上它同样 0 轨，会和 native 形成"两边都说不行"或将来"一边行一边不行"的分裂。

改为：容器判定后，MKV 走新的 native probe（返回 mime/宽高/时长/加密态），
Java 只做展示映射，不再持第二套解析器。**同一个容器只允许一个事实来源。**

## 七、验收

| 判据 | 目标 |
| --- | --- |
| K40 真实来源 2 第 3 项 MKV | 识别 3 轨（HEVC + 2×E-AC3），首帧出现 |
| 起播耗时 | 记录实测值，不设想象上限；与平板 7.7 s 同量级 |
| 平板同源 | r16c 稳态不退化（丢帧/静音账不劣化） |
| 加密轨 | 显式拒绝并给 reason，不解出噪声 |
| 退出 | 无 -10000，无泄漏连接，fd 全部释放 |
| 证据 | `docs/evidence/p240/` 下可复算，含 `demux=` 字段 |

手机通过后仍需独立验证暂停/恢复/seek/长稳——**起播成功不等于播放完成**。

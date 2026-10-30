# P12-C3 · 音频解码与 PCM 供给

> 状态：**已闭环** —— 真实音轨真机出声，人耳确认，单测三平台全过
> 证据：`docs/evidence/p12c/p12c3-aac-48k-stereo-redmi-k40.json`、
> `p12c3-aac-44k1-mono-redmi-k40.json`、`p12c3-no-audio-track-redmi-k40.json`、
> `p12c3-listen-aac-stereo-redmi-k40.json`、`pcm-tests-arm64-k40.txt`
> 设备：Redmi K40（alioth / SM8250 / Android 13 API 33）
> 日期：2026-10-29

---

## 一、链路补上了哪一段

C1/C2 的输出链路一直是"正弦"，没有任何一行代码处理过真实音轨。C3 把前半段接上：

```
文件 fd
  │ MediaExtractorSource（TrackKind::kAudio 选轨）
  ▼
AMediaExtractor ──压缩包──→ MediaCodecAudioDecoder（c2.android.aac.decoder）
  │                              │ PCM 字节（pcm_s16le）
  │                              ▼
  │                       convert_to_float（S16 → float32，纯核心）
  │                              │ float32
  │                              ▼
  │                       SpscAudioRingBuffer（C2，16384 帧）
  │                              │ read
  │                              ▼
  │                       AaudioOutputSession（C1，一行未改）→ 扬声器
  └── 解码线程（AudioDecodeFeeder）拥有前半段，音频实时线程只碰环形缓冲
```

三段各自的归属：

| 层 | 位置 | 为什么在这 |
| --- | --- | --- |
| PCM 描述与转换 | `core/audio/` | 零平台依赖；iOS 的 AudioUnit 同样要 float32 |
| 音频解码 | `backends/mediacodec/decode/` | 平台特性（`AMediaCodec`）只该出现在后端 |
| 解码线程编排 | `probe/audio/` | 目前是探针；C5 之后会迁到 `core/session/` |

## 二、真机结果

### 48 kHz / 立体声 / AAC-LC（10 秒整曲）

| 项 | 实测 |
| --- | --- |
| 轨道 | `audio/mp4a-latm`，声明 48000Hz / 2ch，容器时长 10,021,333µs |
| 解码器 | `c2.android.aac.decoder`（平台自选，未按名指定） |
| 输出格式 | 48000Hz / 2ch / `pcm_s16le`，与声明**一致**（`output_format_diverged=false`） |
| 压缩包 | 提交 470，收块 469 |
| 解出帧数 | **480,256** 帧（写入 480,256，丢弃 0） |
| 环容量 | 16384 帧（≈341ms），环满等待 8820 次（正常背压） |
| 输出协商 | 48000Hz / 2ch / `pcm_float` / low_latency，burst 192（4ms），capacity 1536（32ms） |
| 降级 | `申请独占但实际为 shared`（与 C1 结论一致，已记录） |
| 渲染 | 480,384 帧 / 2658 次回调 / `xrun=0` / 补静音 320 帧 |
| 时长 | wall 10,107ms（源 10,021ms） |

### 44.1 kHz / 单声道（5 秒）

| 项 | 实测 |
| --- | --- |
| 输出协商 | **44100Hz / 1ch**（AAudio 没有强行改成 48k） |
| 解出 / 渲染 | 221,184 / 221,382 帧，`xrun=0`，丢弃 0 |

这一条是**故意选的对照**：如果 AAudio 把 44.1k 顶成 48k，而我们还照 48k 播，
音高会高 8.8%（乐器听得出，人声也听得出）。实测协商结果原样返回 44100，
说明"按片源采样率开流"这条路成立，也就不需要重采样。

### 负向用例

| 输入 | 退出码 | 输出 |
| --- | --- | --- |
| 只有视频轨的 MP4 | 4 | `h264_1080p8.mp4: no audio track among 1 tracks` |
| 不存在的路径 | 3 | `cannot open /data/local/tmp/nope.mp4` |

负向用例进证据，是因为"能播的样品"只证明顺利路径；
判断一条链路是否可信，要看它失败时说不说得清。

## 三、单元测试（P1–P9）

```bash
# 桌面
cmake --build build/vs-debug --target kivo_p12c_pcm_tests --config Debug
./build/vs-debug/backend/Debug/kivo_p12c_pcm_tests.exe
# 真机（arm64）
./build/vs-debug/backend/Release/kivo_p12c_pcm_tests.exe   # Release 连跑 10 次，10/10
```

| 编号 | 内容 | 断言要点 |
| --- | --- | --- |
| P1 | 16bit 定点 → float | 比例必须是 1/32768；`+32767` 不得变成 1.0 |
| P2 | 32bit 定点 → float | 1/2³¹；负满幅 = -1.0 |
| P3 | float32 透传 | 逐位相等，不得引入任何运算 |
| P4 | 半帧被拒 | 3 字节 / 帧长 4 → `kPartialFrame`，且写出 0 帧 |
| P5 | 目标容量不足 | 整块失败，不写目标缓冲 |
| P6 | 无效格式 | 未知编码 / 声道 0 → `kInvalidFormat` |
| P7 | 立体声交错 | 左 0.5 / 右 -0.5 位置不得互换 |
| P8 | 非对齐源地址 | 源指针 +1 字节偏移仍正确（ARM 上真会炸的一条） |
| P9 | 帧长数学 + 零字节 | `frame_size = bytes×channels`；空输入是成功不是错误 |

结果：**9/9 PASS，44 项断言，0 失败**；x86_64 Debug/Release 与 K40 arm64 全过。

P8 只在 arm64 上才有意义：x86 允许非对齐访问，写错了也照样过。
所以这条测试是**在真机上跑才算数**。

## 四、这轮抓到的三个问题

### 问题 1：转换格式该信谁

第一版把"容器声明的格式"传进解码线程，用它去算转换比例与交错步长。
这是错的：容器给的是**期望值**，平台解码器实际输出可能不同。
声道数一旦不符，交错步长就错位——听感不是"格式不对"，而是整段噪声，
而且不会报任何错。

改成每次从 `decoder.output_format()` 现取，并额外断言
`output.channel_count == ring.channel_count()`。不一致直接失败。

### 问题 2：`AMEDIAFORMAT_KEY_PCM_ENCODING` 是 API 28

`minSdk` 是 26。照旧写成常量名，在 API 26/27 上会链接失败。
按既有约定（与 `"rotation"` 同一处理）改用字面量 `"pcm-encoding"`。

另外 `pcm-encoding` **缺失时不能当"未知"处理**：MediaCodec 音频解码器的
默认输出就是 16bit 定点，文档明写。若当未知处理，AAC 播放会在每台设备上都失败。

### 问题 3：丢掉一包就是一秒的静音

`read_next_sample` 会推进解封装器读指针。第一版写法是「读 → 送 → 送失败就继续读下一包」，
`submit` 返回 `TRY_AGAIN` 时那一包就永久丢了。
AAC 一包 1024 帧 ≈ 21ms，丢几包就是可听的断续。

改为：包在手就绝不读下一包（`SampleView` 指向解封装器内部缓冲，
再读一次会把数据覆盖掉），`TRY_AGAIN` 时先收帧、下一轮重试同一包。

## 五、一处必须说清楚的不完美

`frames_silenced` 不是 0，实测 **128 / 320 / 198 帧**（≈2.7–6.7ms），三次不同。

已确认的部分：稳态没有欠载（`xrun=0`、`frames_dropped=0`、
`frames_written == frames_decoded` 严格相等），且这点静音不随播放时长增长
（10 秒与 5 秒的量级一样）——所以它发生在**起流或收尾的瞬间**，不是累积问题。

还没确认的部分：**具体是开头还是结尾**。要分清需要在会话里记录
"第一块非静音出现的帧序号"，而 C2 定过"不要再改 session"，
这一轮没有为它破例。留给 C4 顺手做——C4 本来就要用
`AAudioStream_getTimestamp` 精确定位首帧出声时刻，同一处代码能同时回答这个问题。

## 六、视频链路的回归验证

C3 改了 `mediaextractor_source`（`open_fd` 增加 `TrackKind`，默认仍是 `kVideo`），
这是视频链路也在用的文件。**"编译通过"不能算证据**，所以重装了 APK 并真机跑了一遍：

| 项 | 结果 |
| --- | --- |
| 片源 | `a_hevc_4k10.mp4`（3840×2160 / HEVC Main10 / 4s） |
| 解码器 | `OMX.qcom.video.decoder.hevc` |
| 提交 / 拒绝 | 120 / **0** |
| 渲染帧数 | 120（源 120 帧，零丢） |
| 结果 | `completed=true`、`error=""`、首帧 325ms |

结论：默认参数保住了既有调用点，视频链路未回归。
（`decode_fps` 98.6 与本轮无关——它是 1.2 秒跑完 4 秒素材的瞬时值，
同一台机器不同负载下会在 98–128 之间浮动，不构成性能结论。）

---

## 七、复现

```bash
CXX=/c/Android/sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android26-clang++
ADB=/c/Android/sdk/platform-tools/adb.exe

# 交叉编译（12 个源文件，见 android/app/src/main/cpp/CMakeLists.txt 的 kivo_audio_file_probe）
# 已正式注册为 CMake target，Gradle 产物同样可跑。

MSYS_NO_PATHCONV=1 $ADB push build/p12c/kivo_audio_file_probe /data/local/tmp/
MSYS_NO_PATHCONV=1 $ADB shell 'chmod 755 /data/local/tmp/kivo_audio_file_probe'
MSYS_NO_PATHCONV=1 $ADB shell '/data/local/tmp/kivo_audio_file_probe /data/local/tmp/aac_48k_stereo_lr.mp4'
```

> 片源在 `C:/Android/probe/media/`，用 ffmpeg 的 lavfi 生成：
> - `aac_48k_stereo_lr.mp4`：join 两路正弦，**左 440Hz / 右 880Hz**。
>   左右给不同频率是刻意的——声道顺序一旦错了，听感立刻能分辨，不需要仪器。
> - `aac_44k1_mono.mp4`：1000Hz 单声道，用于验证采样率不被平台改写。

## 八、已知未做（归属明确）

| 项 | 归属 / 理由 |
| --- | --- |
| 重采样 | 不做。Direct Play First：协商不符就显式失败，绝不"顺手重采样一下" |
| 8bit / 24bit packed PCM | 不支持，`map_audio_encoding` 返回 `kUnknown` 让调用方失败 |
| 加密轨道（MediaCrypto） | 显式拒绝并报错，未实现（与视频解码器同一处理） |
| 音频与视频同时播放 | P12-D。目前一次只解一条轨，音视频同步需要两个解封装器 |
| seek 之后的音频重同步 | P12-C6 |
| 音频主时钟（首帧出声时刻） | P12-C4，这正是本节第五节那个问题的答案所在 |
| AAC 之外的实际音轨（AC-3/E-AC-3/FLAC） | 未测。mime 走同一路径，但值得各跑一次再宣称支持 |

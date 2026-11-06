# 211 · 软解兜底 —— 设计稿

> 状态：**设计完成，实现未开始**。总计划 M1 第二张卡（`docs/plan/00-product-master-plan.md` §4）。
> 卡片原文：FFmpeg avcodec 后端进 Android APK（仅解码），路由：硬解支持→硬解，否则软解；
> 验收：RMVB/WMV 测试片可播；两条路径都出版 `decoder.backend`；4K 硬解片不走软解。
> 纪律：本稿先于实现；实现期实测与本稿不符时，更正块留原位（210 先例）。

---

## 一、一句话目标

**MediaExtractor/MediaCodec 播不了的片源（RM/RMVB/WMV，或设备无对应硬解）经 FFmpeg 软解可播；
播得了的原样走硬解，一行行为不变；两条路径的证据都能说出自己是谁（`decoder.backend`）。**

## 二、现状盘点（2026-11-07 实查）

| 资产 | 位置 | 对本卡的意义 |
| --- | --- | --- |
| 桌面 FFmpeg 机器（可移植 C++，`KIVO_ENABLE_FFMPEG` 门控） | `runtime/local_media/local_video_ffmpeg_session.hpp` 聚合 `LocalMediaSourceSession`（demux）+ `FfmpegFormatHandle` + `RealSoftwareDecodeRuntime`（decode），底层在 `third_party_adapter/ffmpeg_adapter` | **demux+软解本体已存在**，不必从零写；实现期第一件事是读这三个单元，确认可复用面与 Android 差异 |
| 桌面 D3D11VA hwaccel | `backends/d3d11/hwaccel/` | 仅桌面，Android 不用（Android 软解=纯 avcodec，不碰 hwaccel） |
| `MediaCodecVideoDecoder` | `backends/mediacodec/decode/mediacodec_video_decoder.hpp`（**具体类，无虚接口**） | 路由不能靠"换实现"；见 §三 D2 |
| `DecodeSession` | `session/decode_session.*`（extractor→decoder→pipeline） | 硬解路径不动；软解是**旁路会话**，不是往里塞 if |
| 能力探测家族 | `backends/mediacodec/device/mediacodec_capability_{probe,inventory}.hpp`（P12-A：77 解码器/41 硬件） | 路由判据的"硬解支持吗"半边复用它 |
| Android FFmpeg 库 | **不存在**（APK 只链 android/log/mediandk/aaudio） | 必须交叉编译；见 §三 D4 |

## 三、设计决策（四条）

### D1 路由：在会话之前，显式判据，理由出版

路由点在 **Kotlin 起播之前**（`PlaybackSession` 打开流程的最前面），不在
`DecodeSession::open()` 里塞 if——后者是硬解会话，往里塞软解等于把两种生命周期搅在一起。

判据按序（全部要能出版到证据的 `routing` 块）：

1. **容器可解**：对 fd 跑一次 `MediaExtractor` 探测（复用现有提取器探测逻辑）——
   0 个视频轨或直接抛错 → 容器不可解（RM/WMV 大概率在这步出局）；
2. **硬解可解**：视频轨 mime 进 `mediacodec_capability_probe` 查软硬解支持
   （P12-A 的能力矩阵逻辑）；支持 → 硬解（**默认路径，行为逐位不变**）；
3. 其余 → 软解，`reason` 写明是哪一步判出来的（G9：不许静默兜底）。

判据 1/2 都过的片源**绝不**走软解（4K 硬解片不走软解的守门就在这里，
与卡片验收第三条一一对应）。

### D2 软解会话：旁路会话，不动硬解接口

**不给 `MediaCodecVideoDecoder` 硬造虚接口**——那是为多态而多态：硬解的
Surface 零拷贝生命周期（`setOutputSurface`、buffer 队列、P16 的 pause/seek 语义）
与软解的帧拉取生命周期几乎没有公共面，硬抽公共基类只会得到一堆 `unsupported`。

软解是**独立的旁路会话**（`FfmpegLocalPlaybackSession`），落在
`runtime/local_media/` 家族（与桌面 `local_video_ffmpeg_session` 同族），内部复用
桌面的 demux+decode 三件套；对外暴露的证据面与硬解会话**同构**：

- 同名计数字段（`frames_received/rendered/dropped`…）、`end_reason`、`outcome`；
- 新增 `decoder.backend = "ffmpeg"`（硬解路径同步出版 `"mediacodec"`——加字段不升版本）；
- seek / pause 首版**不承诺**与硬解等价：能定位就定位，语义差异出版在证据里
  （`seek.backend_semantics`），不静默假装。M1 验收只要求"可播 + 落点正确"。

Kotlin 侧 `PlaybackSession` 按路由结果选会话——JNI 增加独立入口
（`sessionOpenFfmpeg` 等），**不与硬解入口共用句柄**（生命周期不同，共用只会
制造状态机黑洞；P12-D4 的 teardown 表不适用于软解会话）。

### D3 软解帧的呈现：先 sws_scale + ANativeWindow，GL 上传另立卡

软解帧是 CPU 内存（YUV），呈现两条路：

| 路 | 代价 | 取舍 |
| --- | --- | --- |
| **A：`ANativeWindow_lock` + `sws_scale` 直写窗口缓冲** | 每帧一次 CPU 拷贝+转换（1080p 约 5–15 ms，K40 大核可吃下 30fps） | **M1 选它**：改动面最小、可测、数字诚实；RMVB/WMV 内容多为 ≤1080p |
| B：EGL 纹理上传（GLConsumer） | 架构级，动呈现路径 | 另立卡，拿 A 的实测数字决定要不要做 |

A 路的 P12 红线适配：**"视频呈现路径零 CPU 读回"约束的是硬解路径的读回**；
软解路径帧本来就在 CPU（这是软解的定义），`sws_scale` 是**写入**不是读回，
红线不破，但证据里必须写明 `render.sink = "anative_window_cpu"`，
不许让读者以为还是零拷贝直出。

### D4 FFmpeg 进 APK：交叉编译 + 动态库 + 构建脚本入库

- **来源**：FFmpeg 7.1 源码（`curl --noproxy "*"` 下载 tarball，网络坑见交接文档 §4）；
- **配置**：arm64-v8a + NDK r30；`--enable-avformat --enable-avcodec --enable-swscale`
  + RM/RMVB/WMV/FLV/MKV 等容器与 RV/WMA/WMV 系解码器；**LGPL，动态 .so**
  （总计划风险 #2：动态链接 + 构建脚本与来源声明保留）；
- **落位**：构建脚本 `third_party/ffmpeg-android/build.sh` + `README`（来源/配置/许可证），
  产物进 `third_party/ffmpeg-android/dist/`（gitignore，bootstrap 模式对齐桌面 ffmpeg-sdk）；
- **链接**：APK 打包 3–4 个 .so（avformat/avcodec/swscale/avutil），CMake 只在
  `KIVO_ENABLE_FFMPEG=ON`（Android 侧新增开关）时编软解家族——开关关闭时
  APK 与今天逐位相同（**硬解路径零回归的编译期保证**）。

## 四、家族树（落位）

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `third_party/ffmpeg-android/build.sh` + `README.md` | **新** | 交叉编译 FFmpeg 到 Android arm64（脚本幂等，来源与许可证声明） |
| `backends/mediacodec/device/…`（路由判据 1/2 的两个小函数） | 旧（小改） | 容器可解探测 + mime 硬解支持查询（复用能力探测家族，不新造） |
| `runtime/local_media/ffmpeg_local_playback_session.{hpp,cxx}` | **新** | 软解旁路会话：demux→decode→呈现→证据面（复用桌面三件套） |
| `runtime/local_media/ffmpeg_render_sink.{hpp,cxx}` | **新** | sws_scale → ANativeWindow 直写（仅此一件事） |
| `session/decode_session.cxx`（硬解路径出版 `decoder.backend="mediacodec"`） | 旧（一行） | 路径自报 |
| JNI：`jni_ffmpeg_session.{hpp,cxx}` + Kotlin `PlaybackSession` 路由分支 | **新/旧（小改）** | 软解会话的独立入口；路由结果与理由传给 Kotlin |
| `tools/verify_p211_softdec.py` | **新** | 证据校验器 + 变异自检（P16 风格） |
| 测试片 | **新** | RMVB/WMV 样片（公开样片下载或 ffmpeg 自造：`ffmpeg -i 现有片 -c:v rv10/wmv2 -f rm/asf`） |

**实现前必读**（本稿的"先测后定"项）：`third_party_adapter/ffmpeg_adapter.cpp`、
`decode_core/real_software_decode_runtime.*`、`local_media_source_session.*`——
确认三件套的 API 形状与线程模型，D2 的复用面据此定稿，差异写更正块。

## 五、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | RMVB 可播 | RMVB 测试片在真机出画面，`decoder.backend="ffmpeg"`，零崩溃 |
| AC2 | WMV 可播 | 同上（WMV/ASF 容器） |
| AC3 | 硬解零回归 | 既有 HEVC/H.264 片源路径 `decoder.backend="mediacodec"`，帧数闭合与 210 基线逐位相同 |
| AC4 | 4K 不走软解 | 4K 硬解片路由到 mediacodec，`routing.reason` 证明判据 1/2 通过即短路 |
| AC5 | 路由理由 | 每次起播证据带 `routing` 块（走的哪条、为什么），理由非空 |
| AC6 | 门禁 | `verify_p211_softdec.py` 自检变异全抓 |

## 六、不做（范围外）

GL 纹理上传呈现（另立卡）· 软解路径的 pace/avsync 全语义对齐（M1 只要求可播+落点，
差异出版不假装）· 软解 4K（不设目标）· 硬件 hwaccel（MediaCodec 之外的黑魔法不做）·
UI（213 才开始；**Rex UI 拆解推迟到 213 开工时再做**——用户 2026-11-07 决定）。

## 七、开工顺序

1. 读三件套（§四 必读项）→ 更正块定稿 D2 复用面；
2. FFmpeg Android 交叉编译跑通（产物自检：`ffprobe -version` 等价物 + 许可证审计）；
3. 最小可播探针（软解一帧 → ANativeWindow 出图）；
4. 会话 + 路由 + 证据面；
5. 真机取证 AC1–AC6。

---

## 八、复用面定稿（2026-11-07 凌晨实读，§四 必读项完成）

| 单元 | 实测形状 | 结论 |
| --- | --- | --- |
| `cinema_engine/decode_core/real_software_decode_runtime`（370 行） | `open(codec_name)/decode(KivoPacket)/flush/close` + 元数据；**已内置 `decode_video_rgba()`**（sws 转换在Runtime 内） | 直接复用；D3 的呈现 sink 退化为 **RGBA → ANativeWindow 直写**（比 YUV+sws 更薄） |
| `video/source_runtime/local_media/local_media_source_session`（179 行） | demux 会话，产出 `KivoPacket` | 直接复用 |
| `third_party_adapter/ffmpeg_adapter`（661 行 + custom_io） | 唯一碰 `AV*` 的层，自有类型边界（`FfmpegFormatHandle`/`KivoPacket`/`DecodeResult`） | 直接复用；Android 差异（custom_io 的 fd 桥、线程假设）接入时实测，差异写更正块 |

架构红线核对：三件套无任何 Android 依赖（`KIVO_ENABLE_FFMPEG` 门控的纯 C++），
`core/` 不受影响——软解家族落 `runtime/local_media/`（§四 家族树不变，
`ffmpeg_render_sink` 的职责收窄为"RGBA 直写窗口"）。

---

## 九、Android fd 桥定稿（2026-11-07 实读 ffmpeg_adapter/custom_io）

桌面软解路径**本来就是回调 IO**，不走 `fopen`：
`FfmpegAdapter::open_custom_io(FfmpegInputConfig)`（`avio_alloc_context` + 回调，
`ffmpeg_custom_io.cpp`，129 行）+ `LocalMediaSourceSession::ffmpeg_callbacks()`
（`read_callback`/`seek_callback` 静态函数，背后是 `LocalFileSessionStore` + `SourceSession`）。

**Android 侧的接入面因此只有一格**：实现一个 fd 版的 `FfmpegInputCallbacks`
（`pread(fd, …)` + `lseek`，fd/offset/length 语义与 `MediaExtractorSource::open_fd`
完全同款——JNI 传进来的就是这三个值）。预计 ≤60 行，落
`runtime/local_media/ffmpeg_fd_callbacks.{hpp,cxx}`。
`LocalFileSessionStore`/`SourceSession`（providers 族）桌面专用，不复用。

修正 §八 的一处预期：Android 差异不是"custom_io 的 fd 桥待接"，
而是"**回调接口已存在，只差一个 fd 实现**"。

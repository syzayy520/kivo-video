# Kivo Video 后端长期目标

按 Kivo Video 铁律门闸推进后端项目：

1. 从 Core Freeze Spec 和本地记忆中提炼需求与边界。
2. 完成规划设计与自然家族树。
3. 在 `C:\kivo video` 中落地 C++ 后端工程。
4. 先完成 P0 合同冻结，再用 P1 fake backend 驱动完整地基。
5. 保持单文件单职责，禁止平铺文件。
6. 持续验证，直到后端达到可运行、可测试、可维护、与规格一致。


## 当前推进（2026-10-28 起）

后端核心重建计划已定稿，权威文档：

- **`docs/handoff/CURRENT.md`** —— 交接快照（现状 / 怎么跑 / 坑 / 红线），接手先读这份
- **`docs/handoff/TASKS.md`** —— 任务台账（P12-C1…F4），**下一步做什么以它为准**
- `docs/design/backend-core/00-master-plan.md` —— 设计计划书（五条架构决策 + P11-A~P11-G 分阶段验收线）
- `docs/design/backend-core/01-family-tree.md` —— 自然家族树（含每文件职责表与构建注册）
- `docs/design/backend-core/02-p11a-status.md` —— P11-A 实施记录与阻塞项

核心判断：现有 P5 视频平面契约（`DecodedFrame` 携带 `std::vector<uint8_t>` + 强制转 RGBA）
无法承载硬解与 HDR，必须在保留 P2/P3/P4/P6/P7 前提下重建 P5 视频平面。

新增平面命名：`video_plane_gpu`（契约）+ `decode_gpu` / `present` / `sync`（实现）。

铁律补充：私有头文件放 `backend/include_private/video/...` 与 `backend/src/video/...` 镜像，
不得与 .cpp 并排放在 src 下；测试放 `backend/tests/...`（CMake 相对路径以 backend/ 为根）。

## FFmpeg 链接环境（2026-10-28 定稿，勿再走管理员路线）

- 原 SDK `C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1` 的 5 个导入库被保护 DACL 拒绝读取。
- 现用 `third_party/ffmpeg-sdk/` 作为 FFMPEG_ROOT：include/bin 为 junction，lib 由 DLL 导出表重建。
- 重建命令：`third_party/ffmpeg-sdk/bootstrap.cmd "<sdk-root>"`（幂等，无需管理员）。
- 本机 GPU：Intel UHD 630，d3d11va 可用（H.264 4:4:4 除外）。
- 关键判断修正：8.6fps 的根因是呈现路径（sws_scale→RGBA→QImage），不是解码吞吐。
  P11-C 零拷贝上屏优先级上调。

## 平台顺序（2026-10-28 用户指令，最高优先级）

用户明确：**先 Android → 再 iOS → 最后桌面端**。UI 层面之后再谈，当前只做后端核心。

## Android 工具链（2026-10-28 已就绪，免管理员）

全部装在 `C:\Android`，不写系统目录：

- JDK 17.0.13+11 `C:\Android\jdk-17.0.13+11`
- SDK `C:\Android\sdk`（platform-tools 37.0.1 / build-tools **37.0.0** / platforms **android-37.0** /
  ndk **30.0.16248370 r30** / cmake 3.22.1），含旧 r27c 共约 5.4 GB
- Gradle **9.6.1** `C:\Android\gradle-9.6.1`；AGP **9.4.1**（内置 Kotlin 2.3）
- 用户级环境变量已写：JAVA_HOME / ANDROID_HOME / ANDROID_SDK_ROOT + PATH
- **过期提醒**：早期是 Gradle 8.11.1 + r27c + build-tools 35 + android-35，已于 P12-A 期间整体升级；
  若在别处看到旧版本号，以 `06-android-toolchain.md` 为准。

真机闭环已验证：NDK 交叉编译 C++23 → adb push → `/data/local/tmp` 执行成功
（输出 sdk=23 abi=arm64-v8a ptr=8）。

详细记录见 `docs/design/backend-core/06-android-toolchain.md`。

## 测试机现状与结论

- **主开发机（已换，2026-10-28）**：Redmi K40，序列号 `b6e950ee`，`alioth` / M2012K11AC，
  骁龙870（SM8250/kona），**Android 13 API 33**，11.2GB RAM，1080×2400 @60/90/120Hz，
  Adreno 650（GLES 3.2 + Vulkan），**HDR10/HDR10+/HLG/DV 全支持**，BT2020 广色域。
  解码：运行时真实可用的是 **`OMX.qcom.video.decoder.hevc` / `.avc` / `.vp9`**（OMX shim 是主通路），
  另有 `c2.qti.avc.decoder`；**`c2.qti.hevc.decoder` 只在 XML 里声明，运行时未枚举**。
  上限 **4096×2304 / 120Mbps**，4K@60、1080p@240、720p@480，并发 16 实例。
  硬解 HEVC 支持 main / main10 / main10-hdr10 / main10-hdr10plus（10bit 可真机验证）；
  AVC 仅 8bit；VP9 profile0(8)/profile2(10)；**AV1 无硬件，只有 `c2.android.av1.decoder` 软解且上限 2048×2048**。
  软解 HEVC `c2.android.hevc.decoder` **只声明 8bit** → 10bit 片源硬解失败时禁止静默退软解。
- **下限验证机**：Coolpad C106，API 23，硬解上限 1920×1088，无 Vulkan/VP9。
  仅用于 API 23 兼容与降级路径验证，不作主基线。
- 两台机都已跑通 NDK 交叉编译产物（K40 输出 sdk=33 ptr=8）。
- P12-A 能力证据已闭环：真机产出 117 条记录 / 0 解析失败 / 77 解码器 / 41 硬件 / 11 十位。
  证据文件 `docs/evidence/p12a/p12a-capabilities-redmi-k40.json`，
  分析见 `docs/design/backend-core/10-p12a-evidence.md`。
- **P12-B 已闭环（2026-10-29）**：真实硬解 + 零拷贝 Surface 上屏跑通。
  实测 1080p AVC 169.8fps(5.66x)、1080p HEVC10 175.3fps(5.84x)、4K HEVC10 127.4fps(4.25x)，
  三者全部 `samples_submitted == frames_rendered == 轨道帧数`、`samples_rejected == 0`（零丢帧）。
  证据 `docs/evidence/p12b/`，状态见 `docs/design/backend-core/11-p12b-status.md`。
  瓶颈判定：4K 像素是 1080p 的 4 倍但每帧只慢 1.37 倍 → 限制来自每帧固定开销
  （送包/收帧往返、BufferQueue 交互），不是解码吞吐。下一步 P12-C 做呈现节拍 + A/V 同步。
- **P12-C1 已闭环（2026-10-29）**：AAudio 输出骨架真机出声，**用户人耳当场确认**。
  协商 48000Hz/2ch/pcm_float/low_latency/burst 192(4ms)/capacity 1536(32ms)，
  10 秒长跑 xrun=0、零补静音、ratio 1.005。证据 `docs/evidence/p12c/`，
  状态见 `docs/design/backend-core/12-p12c-status.md`。
  两条硬约束：① **启动偏移不是常数**——六次测量 wall 差值 24~157ms（同一 5s 配置
  两次相差 6 倍）、帧侧偏移 36~52ms → 音频时钟零点必须取"首帧实际出声"
  并用 `getTimestamp` 读真实帧位置，**禁止任何固定常数补偿**（抖动与 G7 的 40ms 阈值同阶）；
  ② **EXCLUSIVE 是提示不是契约**，K40 请求独占拿到 shared，必须逐项对照协商结果。
  验收口径：**客观指标脚本断言 + 出声人耳断言**，缺一段不算过（脚本只能证明数据进了 HAL，
  不能排除"进了 HAL 但出口被静音"）。
  新增后端族 `backends/aaudio/`（独立于 mediacodec），探针族 `probe/audio/`。
- **P12-C2 已闭环（2026-10-29）**：SPSC 无锁环形缓冲，9 项单测（R1–R9）433 断言，
  Windows x86_64 Debug/Release 与 **Redmi K40 arm64 真机**各连跑 20 次零失败。
  证据 `docs/evidence/p12c/ring-buffer-tests-arm64-k40.txt`，分析 `13-p12c2-status.md`。
  三条必须记住的：
  ① **缓冲本体在 `core/audio/spsc_audio_ring_buffer`，不在 `backends/aaudio/buffer/`**——
  C2 卡原写于后端目录，实现前已修正；放后端等于绑定 Android，iOS 要么复制要么反向依赖。
  后端只留 `ring_buffer_sample_source.hpp` 薄适配器（core ← 后端，方向不可反）。
  ② **统计字段必须正反两组证据**：只测"实时供给 silenced=0"不能证明计数器有效，
  必须配"半速供给 silenced>0"；实测 104,192 ≈ 242,496−138,400 **闭合**才算证完。
  ③ **供数按绝对时间轴**（`sleep_until`；`sleep_for` 5 秒累积约 40ms 缺口），
  **预灌必须在 `session.start()` 之前**（否则前 13 个 burst = 2496 帧被记成欠载）。
  并发测试必须在 arm64 真机跑：x86 强内存序会放过缺失的 acquire/release。
  构建坑：Git Bash 的 `HTTP_PROXY` 与 `http_proxy` 并存会让 MSBuild 报 `MSB6001`，
  构建前 `unset http_proxy https_proxy`。

- **P12-C3 已闭环（2026-10-29）**：真实音轨解码出声，**用户人耳确认**。
  链路 `MediaExtractorSource(kAudio) → MediaCodecAudioDecoder → convert_to_float → SpscAudioRingBuffer → AaudioOutputSession`。
  真机 AAC 48k/2ch 10 秒整曲：解出 480,256 帧、写入 480,256、丢弃 0、`xrun=0`、
  补静音 320 帧（仅边界）；44.1k/单声道协商为 44100Hz/1ch（AAudio 未顶成 48k）。
  证据 `docs/evidence/p12c/p12c3-*.json`、`pcm-tests-arm64-k40.txt`，分析 `14-p12c3-status.md`。
  四条必须记住的：
  ① **音频允许 CPU 拷贝**，G5 零拷贝只约束视频呈现路径，别拿它卡音频；
  ② **转换格式只能取 `decoder.output_format()`**，用容器声明值会掩盖平台改格式，
  声道数不符时交错步长错位 → 整段噪声且不报错；
  ③ **不做重采样**：协商不符即显式失败（退出码 7），Direct Play First；
  ④ **`submit` 返回 TRY_AGAIN 时绝不能读下一包**——读一次就推进读指针，
  那一包永久丢失（AAC 一包≈21ms，几包就是可听断续）。
  另外：C2 那条"按绝对时间轴供数"**只适用于无缓冲直连**；有环之后节奏由环决定。

## 架构铁律（P12-A 真机数据逼出来的，不许违背）

1. **绝不按 codec 名选解码器**。XML 声明的 ≠ 运行时存在的（`c2.qti.hevc.decoder` 即反例），
   只能靠运行时枚举 + 能力匹配。
2. **不许有"Codec2 比 OMX 新"的偏见**。本机 HEVC 硬解只有 OMX 路径，
   按 `c2.*` 前缀筛会把 HEVC 硬解直接筛没。筛选条件只写能力，不写框架、不写命名。
3. **能力上限取同硬件各组件的最小值**。同一 HEVC 核：OMX 报 8192、Codec2 报 4096×2304、
   XML 声明 4096×2304 → 按 4096×2304 判。信大值会"打开成功但首帧解码失败"，比直接拒绝更糟。

## 踩坑记录（Android）

- adb 首次连接为 `unauthorized`：删除并重建 `~/.android/adbkey`（原 key 已备份到 `keybackup-20261028`）后重新弹窗，手机点允许即可。
- bash 下 `adb push` 目标路径会被 MSYS 转换，必须用 PowerShell 或 `MSYS_NO_PATHCONV=1` 且本地路径写 `C:/...`。
- PowerShell 里 `adb` 的输出走 stderr，会触发 NativeCommandError，属正常噪声。
- **HyperOS（K40）会拒绝 `adb install`**：报 `INSTALL_FAILED_USER_RESTRICTED`。
  绕法：`adb push` APK 到 `/data/local/tmp/`，再 `adb shell "pm install -r -t <path>"` → Success。
- NDK 根本没有 codec 枚举/能力查询 API（`AMediaCodecList` 不存在）。
  枚举必须走 Java 层 `MediaCodecList`，C++ 侧只做解析与聚合。
  **修正（2026-10-29）**：NDK r30 新增了 `AMediaCodecStore`，但全部 `__INTRODUCED_IN(36)`，
  即 Android 16 才有。minSdk 26 + 测试机 API 33 用不了 → Java 路径仍是唯一选择。
- **`AMediaExtractor_setDataSource(路径)` 在本机返回 `AMEDIA_ERROR_MALFORMED(-10002)`**，
  文件本身完好（md5 一致、ffprobe 可解）。生产代码一律用
  `AMediaExtractor_setDataSourceFd(fd, 0, length)`，路径只作诊断显示名。
- **这台 K40 的 logcat 对 adb 完全不可见**（`adb logcat -d` 返回 0 行，HyperOS 关了 logd）。
  所有 `__android_log_print` 等于写黑洞 → 失败原因必须走句柄回传 + 落盘 JSON 证据。
- minSdk 26 撞上的 API 28 符号：`AMediaCodec_getName/ReleaseName`（用 dlsym 降级）、
  `AMEDIAFORMAT_KEY_CSD_0` / `AMEDIAFORMAT_KEY_ROTATION`（用字面量 "csd-0"/"rotation"）、
  `AMediaFormat_copy`（API 29，**没有替代，不要试图克隆格式**）。
- 解封装器标志位不能直接透传给解码器：extractor 的 `ENCRYPTED=2` 撞上 codec 的
  `CODEC_CONFIG=2`，会把真实数据当配置包丢掉。必须经中立语义翻译。
- **`AMEDIAFORMAT_KEY_PCM_ENCODING` 也是 API 28**，minSdk 26 一律用字面量 `"pcm-encoding"`。
  且该键**缺失时必须按 16bit 处理**（MediaCodec 音频解码器默认输出，文档明写），
  当"未知"处理会让 AAC 在每台设备上都失败。
- **向 `adb shell` 传重定向要整段单引号**：`$ADB shell "... > /data/local/tmp/x"` 会被
  本地 Git Bash 吃掉 `>`，报"本地路径不存在"而远端什么都没收到。
- **构建前 `unset http_proxy https_proxy`**：本机 `HTTP_PROXY` 与 `http_proxy` 并存，
  MSBuild 会报 `MSB6001 CL.exe 命令行开关无效`，与实际原因毫无关系。
- `build/ninja-debug` 的 ninja 路径指向旧用户（Administrator），已失效；
  桌面构建用 `build/vs-debug`（VS 生成器，1.9GB 真实产物），`cmake --build build/vs-debug --target <t> --config Debug`。

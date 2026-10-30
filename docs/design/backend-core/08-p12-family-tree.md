# P12 Android 家族树

- 版本：V1.0
- 日期：2026-10-28
- 关联：`07-p12-plan.md`
- 命名：目录 = 语义域，文件 = 单一职责，`snake_case`（C++）/ `PascalCase`（Kotlin）

---

## 一、设计原则（沿用 P11，追加两条）

1. 同一家人住一起：设备只进 `device/`，解封装只进 `source/`，解码只进 `decode/`，呈现只进 `present/`。
2. 长出新职责才分家：不预造空目录，子域文件数 ≥ 3 且职责可再分时才开子目录。
3. 一个文件一件事：文件名即职责说明。
4. 契约与实现分离：`include/` 只放契约，`src/` 只放实现。
5. **新增**：Kotlin 层只做「生命周期 + Surface 传递 + JNI 转发」，**不许写业务逻辑**。
6. **新增**：`core/` 零平台头文件，由守卫脚本拦截。

---

## 二、工程族：`android/`

```
android/
├── settings.gradle.kts                 # 模块声明与仓库
├── build.gradle.kts                    # 根构建：插件版本统一
├── gradle.properties                   # 全局属性（ABI、并行、内存）
├── gradle/
│   ├── libs.versions.toml              # 版本目录（单一真相）
│   └── wrapper/gradle-wrapper.properties
└── app/
    ├── build.gradle.kts                # app 模块 + externalNativeBuild
    └── src/main/
        ├── AndroidManifest.xml         # 仅权限与 Activity 声明
        ├── java/com/kivo/cinema/
        │   ├── ui/
        │   │   ├── PlayerActivity.kt   # 宿主：生命周期 + 挂 SurfaceView
        │   │   └── PlayerSurfaceView.kt# 持有 Surface，变化时通知 native
        │   └── bridge/
        │       └── NativePlayer.kt     # JNI 唯一入口：System.loadLibrary + native 声明
        └── cpp/
            └── CMakeLists.txt          # 聚合：引用 backend/ 源码
```

### 文件职责表

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `settings.gradle.kts` | 声明模块与仓库 | 不含任何依赖版本 |
| `build.gradle.kts`（根） | 统一插件版本 | 不含 app 配置 |
| `libs.versions.toml` | 版本号单一真相 | 不含构建逻辑 |
| `app/build.gradle.kts` | app 模块配置与 CMake 接线 | 不含依赖版本字面量 |
| `AndroidManifest.xml` | 权限与组件声明 | 不含业务配置 |
| `PlayerActivity.kt` | 生命周期与 Surface 挂载 | **不碰解码/渲染逻辑** |
| `PlayerSurfaceView.kt` | Surface 生命周期回调 | 不持有播放状态 |
| `NativePlayer.kt` | native 方法声明与库加载 | 不做类型转换以外的计算 |
| `cpp/CMakeLists.txt` | 聚合后端源码、链接系统库 | 不含源码实现 |

---

## 三、C++ 核心族：`backend/src/video/video_plane/`

```
video_plane/
├── core/                               # 可移植核心（三端共用；P12-C 起填充）
│   ├── audio/                          # P12-C2/C3
│   │   ├── spsc_audio_ring_buffer.{hpp,cxx}   # C2：无锁、零分配、短写不覆盖
│   │   ├── pcm_format.hpp                     # C3：PCM 描述（纯数据，无平台类型）
│   │   └── pcm_format_converter.{hpp,cxx}     # C3：S16/I32 → float32，逐帧
│   ├── source/                         # 解封装抽象
│   ├── packet/                         # 包队列与背压
│   ├── frame/                          # 帧队列（有界、可归还）
│   ├── clock/                          # 主时钟（P12-C4 已落地为 audio/clock/）
│   ├── pace/                           # 帧节拍（P12-C5 已实现，见 三之四）
│   ├── sync/                           # A/V 同步
│   └── session/                        # 播放会话状态机
├── backends/
│   ├── d3d11/                          # 已有，Windows 参考后端
│   └── mediacodec/                     # Android 视频后端（P12 新建；音频后端见"三之二"）
│       ├── device/                     # 能力证据链路（P12-A，已实现）
│       │   ├── mediacodec_codec_profile_level_ids.hpp   # 平台常量表（纯数据）
│       │   ├── mediacodec_codec_record.hpp              # 记录结构（纯数据）
│       │   ├── mediacodec_codec_record_parser.cxx       # TAB 记录 → 结构
│       │   ├── mediacodec_codec_profile_namer.cxx       # profile/level → 可读名
│       │   ├── mediacodec_codec_bit_depth.cxx           # profile → 位深
│       │   ├── mediacodec_codec_implementation_class.cxx# codec 名 → 实现来源
│       │   ├── mediacodec_system_property_reader.cxx    # 系统属性读取（唯一入口）
│       │   ├── mediacodec_device_identity.cxx           # 设备身份采集
│       │   ├── mediacodec_wall_clock.cxx                # UTC 毫秒时间戳
│       │   ├── mediacodec_json_string_escaper.cxx       # JSON 字符串转义
│       │   ├── mediacodec_capability_inventory.cxx      # 去重/排序/计数
│       │   ├── mediacodec_capability_json_renderer.cxx  # 纯函数 JSON 渲染
│       │   └── mediacodec_capability_probe.cxx          # 编排：聚合→渲染→落盘
│       ├── source/                     # 解封装（P12-B/P12-C3，已实现）
│       │   ├── extract_outcome.hpp              # 解封装结果码（纯数据）
│       │   ├── media_track_info.hpp             # 视频轨静态信息（纯数据）
│       │   ├── track_kind.hpp                   # 轨道种类（C3 新增）
│       │   ├── audio_track_info.hpp             # 音频轨静态信息（C3 新增，纯数据）
│       │   ├── sample_view.hpp                  # 中立样本视图 + 标志位语义
│       │   └── mediaextractor_source.cxx        # fd 打开 / 按 kind 选轨 / 取包 / seek
│       ├── decode/                     # 解码（P12-B 视频 / P12-C3 音频）
│       │   ├── mediacodec_format_guard.cxx      # 配置前校验（不克隆格式）
│       │   ├── mediacodec_codec_name_resolver.cxx # API 28 符号运行时降级
│       │   ├── mediacodec_video_decoder.cxx     # 送包 / 收帧 / EOS / flush
│       │   ├── mediacodec_output_pcm_format.{hpp,cxx}  # C3：输出格式 → PcmFormat
│       │   └── mediacodec_audio_decoder.{hpp,cxx}      # C3：音频 → PCM 字节
│       ├── present/                    # 上屏（P12-B，已实现）
│       │   └── surface_window_binder.cxx        # ANativeWindow RAII
│       ├── session/                    # 编排（P12-B 起；P12-C6 拆为 worker 族）
│       │   ├── decode_stats.hpp                 # 统计事实（纯数据）
│       │   ├── decode_stats_renderer.cxx        # 统计 → JSON（纯函数）
│       │   ├── decode_pipeline_gate.cxx         # C6：停止标志 + 输入空位信号 + 送包收工
│       │   ├── decode_stage_cost.cxx            # C6：单段平台调用 次数/累计/峰值
│       │   ├── decode_failure_slot.cxx          # C6：首个失败原因槽
│       │   ├── decode_pace_sampler.cxx          # C6：节拍摘要的跨线程发布
│       │   ├── decode_submit_worker.cxx         # C6：送包循环（独占 extractor）
│       │   ├── decode_receive_worker.cxx        # C6：收帧 + 节拍 + 释放（独占 pacer）
│       │   └── decode_session.cxx               # C6 起退化为编排者，不含循环体
│       └── present/gles_oes_presenter.cxx       # OES 采样 + 着色器（P12-C 才需要）
└── probe/                              # 跨平台探针
```

### 文件职责表（P12-A 范围，已实现）

| 文件 | 唯一职责 | 允许依赖 |
| --- | --- | --- |
| `mediacodec_codec_profile_level_ids.hpp` | 固化 AOSP `CodecProfileLevel` 常量 | 无（纯数据） |
| `mediacodec_codec_record.hpp` | 定义一条 codec 记录 | 无（纯数据） |
| `mediacodec_codec_record_parser.cxx` | TAB 记录 → `CodecInventoryRecord` | 上述 |
| `mediacodec_codec_profile_namer.cxx` | profile / level → 可读名 | 常量表 |
| `mediacodec_codec_bit_depth.cxx` | (mime, profile) → 位深；0 表示未知 | 常量表 |
| `mediacodec_codec_implementation_class.cxx` | codec 名 → software / vendor / unknown | 无 |
| `mediacodec_system_property_reader.cxx` | 读系统属性，**唯一** include `<sys/system_properties.h>` | 平台头 |
| `mediacodec_device_identity.cxx` | 采集设备身份九项 | 属性读取 |
| `mediacodec_wall_clock.cxx` | 当前 UTC 毫秒 | 无 |
| `mediacodec_json_string_escaper.cxx` | 字符串 → 可安全嵌入 JSON 的形式 | 无 |
| `mediacodec_capability_inventory.cxx` | 原始记录 → 去重排序计数后的清单 | parser + bit_depth |
| `mediacodec_capability_json_renderer.cxx` | 清单 + 身份 + 时间戳 → JSON 文本（**纯函数**） | 上述 |
| `mediacodec_capability_probe.cxx` | **只做编排**：聚合 → 渲染 → 落盘 | 上述全部 |

> P12-A 的第一版把解析、命名、位深、分类塞进一个文件，把 JSON 转义、属性读取、
> 聚合、渲染、落盘塞进另一个文件——两处都是堆砌，已按铁律拆开。
> 拆分后行为完全一致：真机复跑 `117 / 117 / 0 / 77 / 41 / 11` 六个数字不变。

### 文件职责表（P12-B 范围，已实现）

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `media_track_info.hpp` | 描述选中视频轨的静态信息 | 不含平台句柄 |
| `sample_view.hpp` | 描述一包压缩数据（中立语义标志） | 不含平台常量 |
| `extract_outcome.hpp` | 解封装结果码 | 不由它做恢复决策 |
| `mediaextractor_source.cxx` | 打开 fd、选视频轨、按序交包、seek | 不解码、不计时 |
| `mediacodec_format_guard.cxx` | 配置前校验格式必需字段 + 可读描述 | **不克隆格式**（minSdk 26 无拷贝 API） |
| `mediacodec_codec_name_resolver.cxx` | 取平台实选 codec 名，API 28 以下返回空 | **不得用于任何逻辑判断** |
| `mediacodec_video_decoder.cxx` | 送包 / 收帧（**不自动 release**）/ EOS / flush / 取 codec 名 | 不读文件、不决策丢帧 |
| `decoded_frame.hpp` | 一帧的描述（缓冲索引 / pts / EOS 标志） | 不含缓冲本身、不做呈现决策 |
| `surface_window_binder.cxx` | ANativeWindow 的 RAII 释放 | 不做 Java→native 转换（在 JNI 层） |
| `decode_stats.hpp` | 统计事实（计数、耗时、节拍快照、终态） | 不含达标判定 |
| `decode_pace_stats.hpp` | 节拍度量（不带直方图，快照只有几百字节） | 不做统计计算 |
| `decode_pace_report.cxx` | `PaceSummary → DecodePaceStats` 的唯一转换点 | 不读时钟、不写文件 |
| `decode_stats_renderer.cxx` | 统计 + 时间戳 → JSON（**纯函数**） | 不读时钟、不写文件 |
| `decode_session.cxx` | 线程、流水线编排、节拍接线（wait/drop）、无锁计数、失败归因 | 不做 A/V 偏差统计、不做合成器排程 |

> **P12-C5 把"收"和"呈现"拆开了**：`receive()` 不再内部 `releaseOutputBuffer(render=true)`，
> 而是返回 `DecodedFrame`，由会话按节拍决定 `release_frame(index, render)`。
> 理由很直接：`render` 参数是 MediaCodec 上唯一能表达"丢弃"的开关；
> 藏在解码器内部，会话就永远没有丢帧的能力。

> P12-B 的关键取舍：**热路径不放锁**。
> 第一版每渲染一帧就锁一次互斥量并复制两个 `std::string`，
> 等于给解码器挂了个秤砣；改为原子计数后 1080p HEVC 提升 21%。

### 文件职责表（P12-C3 范围，音频解码链路）

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `source/track_kind.hpp` | 枚举"要视频轨还是音频轨" | 不含平台常量 |
| `source/audio_track_info.hpp` | 描述音频轨静态信息 | 不含平台句柄 |
| `source/mediaextractor_source.cxx`（改） | 按 `TrackKind` 选一条轨并取包 | 不解码、不计时 |
| `decode/mediacodec_output_pcm_format.cxx` | 读输出 `AMediaFormat` → `PcmFormat`（**纯函数**） | 不做格式转换 |
| `decode/mediacodec_audio_decoder.cxx` | 送包 / 收 PCM / EOS / flush | 不写缓冲、不转格式、不计时 |
| `core/audio/pcm_format.hpp` | PCM 描述（采样率/声道/编码） | 不含平台类型 |
| `core/audio/pcm_format_converter.cxx` | S16/I32 → float32，逐帧整帧转换 | 不做重采样、不做声道映射 |

> **为什么 PCM 转换放 `core/`**：iOS 的 AudioUnit 同样要 float32，
> 而 MediaCodec 输出的是 S16/PCM-16bit。放后端就等于让 iOS 再写一遍。
> **为什么不做重采样**：Direct Play First。采样率不符必须显式失败并记录，
> 绝不"顺手重采样一下"——那是最典型的静默降质。

### 文件职责表（P12-C4 范围，音频时钟的纯逻辑部分）

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `core/audio/clock/audio_clock_types.hpp/.cxx` | 来源枚举 + 快照结构 + 可读名 | 不读数、不做判断 |
| `core/audio/clock/audio_clock_source.hpp` | 注入接口（C5 只依赖这一层） | 不含任何实现 |
| `core/audio/clock/monotonic_clock_guard.cxx` | 对外时间只增不减（拒绝倒退、钳制跳变） | 不构造时钟、不认识平台 |
| `core/audio/clock/frame_position_extrapolator.cxx` | (frame,time) → now 的位置外推；采样率切换保持位置连续 | 不读设备 |
| `core/audio/clock/system_clock_source.cxx` | 无音轨时的独立单调时钟 | 不冒充音频时钟（自报 `system`） |

> **为什么这四组放 `core/`**：见本节三之二末尾的修正说明。判据只有一条 ——
> **它认不认识平台类型**。不认识就进 core，iOS 与桌面共用同一份。
> 现成证据：同一份源在 MSVC 与 NDK arm64 上分别编译，66 项断言全过
> （`kivo_p12c4_clock_tests`）。

### 私有头（镜像 src，不放 src 下）

```
backend/include_private/video/video_plane/backends/mediacodec/
├── device/mediacodec_codec_profile_level_ids.hpp
├── device/mediacodec_codec_record.hpp
├── device/mediacodec_codec_record_parser.hpp
├── device/mediacodec_codec_profile_namer.hpp
├── device/mediacodec_codec_bit_depth.hpp
├── device/mediacodec_codec_implementation_class.hpp
├── device/mediacodec_system_property_reader.hpp
├── device/mediacodec_device_identity.hpp
├── device/mediacodec_wall_clock.hpp
├── device/mediacodec_json_string_escaper.hpp
├── device/mediacodec_capability_inventory.hpp
├── device/mediacodec_capability_json_renderer.hpp
├── device/mediacodec_capability_probe.hpp
├── source/extract_outcome.hpp
├── source/media_track_info.hpp
├── source/track_kind.hpp
├── source/audio_track_info.hpp
├── source/sample_view.hpp
├── source/mediaextractor_source.hpp
├── decode/mediacodec_format_guard.hpp
├── decode/mediacodec_codec_name_resolver.hpp
├── decode/mediacodec_video_decoder.hpp
├── decode/mediacodec_output_pcm_format.hpp
├── decode/mediacodec_audio_decoder.hpp
├── present/surface_window_binder.hpp
├── session/decode_stats.hpp
├── session/decode_stats_renderer.hpp
└── session/decode_session.hpp
```

> **注意：没有 `mediacodec_format_builder.hpp`，也没有 `mediacodec_codec_selector.hpp`。**
> 前者原计划"克隆格式再改"，查证发现 `AMediaFormat_copy` 是 API 29 而 minSdk 是 26，
> 会写出在 API 26/27 上必崩的代码，已改为 `mediacodec_format_guard`（只校验、不克隆）。
> 后者改为 `mediacodec_codec_name_resolver`（只取平台实选名，不按名选解码器）。
> 若在别处看到这两个旧名字，以本节为准。

> **没有 `mediacodec_enumerator.cxx`。** 截至 NDK r27，NDK 未提供任何 codec 枚举或能力查询 API
> （`AMediaCodecList` 在 NDK 中不存在），枚举只能走 Java 层 `MediaCodecList`。
> **修正（P12-B）**：NDK r30 确实新增了 `AMediaCodecStore`，但每个函数都标 `__INTRODUCED_IN(36)`，
> 即 Android 16 才有；minSdk 26 + 测试机 API 33 仍用不了 → **Java 路径依旧是唯一选择**。
> 这条约束写死在家族树里，避免以后有人再去 NDK 里翻不存在的接口。

> 文件后缀说明：Android 后端使用 `.cxx` 以便与桌面端 `.cpp` 在同一 CMake 树中区分来源，
> 也让「这段代码只在 Android 上编译」在文件名层面可见。

### 三之二、音频后端族：`video_plane/backends/aaudio/`（P12-C1 已实现；clock/ 见 C4）

```
backends/aaudio/
├── device/                             # 开流与协商（已实现）
│   ├── aaudio_stream_config.hpp        # 请求参数（纯数据）
│   ├── aaudio_open_outcome.hpp         # 实际协商结果 + 降级原因（纯数据）
│   ├── aaudio_format_names.cxx         # 枚举 → 可读名（证据用，纯函数）
│   └── aaudio_stream_opener.cxx        # builder → open + EXCLUSIVE→SHARED 降级链
├── buffer/                             # 线程间通道的**接入端**（P12-C2 已实现）
│   └── ring_buffer_sample_source.hpp   # 适配器：core 缓冲 → Session::SampleSource
├── clock/                              # 时钟的**平台层**（P12-C4 已实现）
│   ├── aaudio_timestamp_reading.hpp/.cxx   # 单次读取结果 + 状态分类（纯数据）
│   ├── aaudio_clock_probe.hpp              # 读数接口（让回退分级能在桌面单测）
│   ├── aaudio_timestamp_reader.hpp/.cxx    # 唯一调用 getTimestamp / getFramesRead/Written 处
│   ├── aaudio_clock_stats.hpp              # 回退/拒绝/外推计数 + dominant_source
│   ├── aaudio_monotonic_now.hpp/.cxx       # CLOCK_MONOTONIC 取时刻（统一尺子）
│   └── aaudio_clock_source.hpp/.cxx        # 编排：读 → 外推 → 守卫 → 快照
└── session/                            # 编排（已实现）
    ├── aaudio_output_session.cxx       # 启停 + 数据回调（回调内零分配、零锁）
    └── audio_output_stats.hpp          # 统计事实（纯数据：xrun / 回退次数 / 欠载）
```

> **`buffer/` 里没有环形缓冲本体。** 缓冲在 `core/audio/spsc_audio_ring_buffer`，
> 这里只放把它接到 AAudio 接口上的薄适配器。
> 缓冲本体若放后端目录，iOS 的 AudioUnit 要么复制一份、要么反向依赖 Android 目录，
> 两条都是死路。（C2 卡原写于此，实现前已修正，见 `13-p12c2-status.md` §一。）

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `aaudio_stream_config.hpp` | 描述我们把要什么（采样率/格式/通道/性能模式） | 不含平台句柄 |
| `aaudio_open_outcome.hpp` | 描述实际拿到什么 + 为何降级 | 不做重试决策 |
| `aaudio_format_names.cxx` | 枚举值 → 可读名 | 不读设备、不落盘 |
| `aaudio_stream_opener.cxx` | 按降级链开流，失败带 reason | 不管启停、不管数据 |
| `ring_buffer_sample_source.hpp` | core 缓冲 → `SampleSource` 的一次转接 | 不实现缓冲、不做格式转换 |
| `aaudio_timestamp_reading.hpp/.cxx` | 一次 `getTimestamp` 的结论（ok / 未启动 / 暂不可用 / 超时 / 断开） | 不重试、不决策 |
| `aaudio_clock_probe.hpp` | 时钟读数接口 | 不含实现 |
| `aaudio_timestamp_reader.hpp/.cxx` | 调 AAudio 时钟 API 并翻译错误码 | 不计算、不外推 |
| `aaudio_clock_stats.hpp` | 计数事实 + 主导来源判定 | 不含达标判定 |
| `aaudio_monotonic_now.hpp/.cxx` | 取与 `timeNanoseconds` 同源的单调时刻 | 不做时钟逻辑 |
| `aaudio_clock_source.hpp/.cxx` | 编排四段，产出 `AudioClockSnapshot` | 不判断"该不该显示这一帧" |
| `aaudio_output_session.cxx` | 启停与数据回调 | 不解析片源、不解码 |
| `audio_output_stats.hpp` | 统计事实（纯数据） | 不含达标判定 |

> **红线（P12-C 特有）**：AAudio 数据回调里**不许加锁、不许 new、不许打日志、不许构造字符串**。
>
> **修正（P12-C4 实现时，2026-10-29）**：本节原写"`clock_monotonicity_guard` 必须放在
> `backends/aaudio/clock/` 而不是 `core/clock/`，因为它是平台时钟的修补"。
> 实现后判定这条**不成立**，已移到 `core/audio/clock/`，理由三条：
>
> 1. 守卫的输入只有 `int64` 纳秒，**不认识任何平台类型**。判据应当是"认不认识平台"，
>    不是"是不是在修补时钟"；
> 2. 实测回退**不是平台怪癖**：同一台 K40，48k 每次 2–3 次、44.1k 每次 8 次，
>    两种采样率都会抖（数据见 `16-p12c4-status.md` §三·2）；
> 3. iOS 的 `AudioTimeStamp` 同样需要"外推 + 单调守卫"，放后端等于让 iOS 再写一遍。
>
> 这与 C2 把环形缓冲从 `backends/aaudio/buffer/` 移到 `core/audio/` 是同一条理由。
> `backends/aaudio/clock/` 只保留真正绑 AAudio 的部分：读时间戳、翻译错误码、算 latency。

> **为什么音频后端是独立语义域而不是塞进 `mediacodec/`**：
> 解码用 MediaCodec，输出用 AAudio，两者生命周期、线程模型、失败模式都不同。
> 塞进一个目录会让「音频为什么卡」和「视频为什么掉帧」混在一起无法定位。

### 三之三、音频探针族：`video_plane/probe/audio/`（P12-C1 ~ C3）

```
probe/audio/
├── aaudio_tone_source.hpp/.cxx      # 正弦样本源（实现 Session::SampleSource）
├── tone_ring_producer.hpp/.cxx      # 模拟解码线程：按倍速把正弦灌进环形缓冲
├── aaudio_probe_report.hpp/.cxx     # 正弦报告 → JSON（纯函数，可直接 golden 测试）
├── audio_decode_feeder.hpp/.cxx     # C3：真实解码线程（解封装→解码→转 float→写环）
├── audio_file_probe_report.hpp/.cxx # C3：文件播放报告 → JSON（纯函数）
├── aaudio_tone_main.cxx             # 入口 A：参数 → 会话 → 出声 → 出证据（正弦/direct/ring）
└── audio_file_probe_main.cxx        # 入口 B：文件 → 音频解码 → 环 → 出声 → 出证据
```

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `aaudio_tone_source.cxx` | 按采样率/声道生成正弦 | 不碰流、不碰设备 |
| `tone_ring_producer.cxx` | 按倍速、按绝对时间轴供数，并计数短写 | 不做解码、不判断欠载是否达标 |
| `aaudio_probe_report.cxx` | 报告结构 → JSON 文本 | 不读设备、不读时钟、不判定达标 |
| `audio_decode_feeder.cxx` | 在解码线程里跑：取包→送包→收 PCM→转 float→写环 | 不打开文件、不开流、不做 A/V 同步 |
| `audio_file_probe_report.cxx` | 报告结构 → JSON 文本 | 不读设备、不读时钟、不判定达标 |
| `audio_file_probe_main.cxx` | 编排：打开文件 → 建会话 → 起解码线程 → 等结束 → 出证据 | 不含解码细节、不含转换数学 |

> **两个入口而不是给一个入口加第 N 个模式**：C1/C2 的正弦入口参数是
> 「时长 + 频率 + 供给方式」，C3 的文件入口参数是「文件路径 + 循环」，
> 参数语义完全不同。硬塞成一个入口会让 `argv` 变成一串互相排斥的开关，
> 每个分支都要判"这个参数在这个模式有没有意义"。

> **供给方式由参数选（`direct` / `ring` + 倍速），不是一个入口一个二进制**：
> 两者除"谁供数"之外完全一致，拆开就得复制报告与退出码逻辑，两份真相迟早漂移。
>
> `tone_ring_producer` 存在的理由不是"能出声"，而是**能造出欠载**：
> 一个从来没非零过的 `frames_silenced` 分不清是"真的没欠载"还是"根本没在计"。
> 半速供数（0.5x）就是用来把这个计数器证伪的。

> **为什么是独立可执行进程，而不是在 App 里加个按钮**：
> AAudio 在 shell 进程同样可用，验证音频链路不需要改一行 Kotlin，
> 也不会把测试代码混进播放器。代价是权限要自己 `chmod 755`（`adb push` 会重置权限）。
>
> `probe/` 下原有的 `.cpp` 是跨平台解码探针（走 FFmpeg）；
> `probe/audio/` 是 Android 专用（`.cxx` + `<aaudio/AAudio.h>`），
> 后缀不同即"只在这个平台编译"在文件名层面可见。

### 三之四、节拍器族：`video_plane/core/pace/`（P12-C5）

```
core/pace/
├── pace_action.hpp/.cxx        # 动作与时钟健康度枚举 + PaceDecision（纯数据）
├── pace_policy.hpp/.cxx        # 阈值 + decide() 纯函数（可对表格逐条断言）
├── pace_histogram.hpp/.cxx     # 有符号定宽直方图（1026 桶，零分配）
├── pace_stats.hpp/.cxx         # 聚合计数与分位数
├── clock_position_tracker.hpp/.cxx  # 陈旧外推 / 跳变重锚 / 倒退钳制
├── frame_pacer.hpp/.cxx        # 编排：pts → due_at → 动作 → 等待 → 记账
└── pace_now.hpp/.cxx           # NowFn / WaitFn（可注入，单测不真睡）
```

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `pace_action.cxx` | 动作/健康度的可读名（进证据 JSON） | 不做判定 |
| `pace_policy.cxx` | `(迟到量, 连迟帧数, 是否追帧) → 动作` | 不读时钟、不睡觉、不记账 |
| `pace_histogram.cxx` | 桶计数、分位数、超阈值计数 | 不做业务判断 |
| `pace_stats.cxx` | 把观察值聚合成 `PaceSummary` | 不碰时钟 |
| `clock_position_tracker.cxx` | 把 `AudioClockSource` 变成"可信、非递减的位置" | 不算 `due_at` |
| `frame_pacer.cxx` | 建锚点、算 `due_at`、编排等待与记账 | 不碰 MediaCodec、不认识 `ANativeWindow` |
| `pace_now.cxx` | `steady_clock` 纳秒 + `sleep_until` | 不含策略 |

> **为什么整套放 `core/` 而不是 `backends/mediacodec/`**：节拍是"什么时候该显示这一帧"，
> 与像素怎么上屏无关。iOS / 桌面要用同一份实现，放后端就等于让它们各写一遍。
> `grep -rn "android/\|GLES\|aaudio" core/pace/` 为空 —— 这条可直接复验。

> **节拍器依赖 `core::audio::AudioClockSource` 不是"节拍依赖音频"**：
> 接口只有 `now() → {位置, 陈旧, 来源}`；纯视频时注入的是 `SystemClockSource`，
> 证据里 `clock_source` 会写 `system`。新建一套平行接口才会有两套"媒体位置"语义，
> C8 算 A/V 偏差时必然对不上。

### 三之五、分线程流水线族（P12-C6，已收口但默认回退）

```
core/pipeline/                      # 纯逻辑，零平台依赖
├── submit_types.hpp/.cxx           # 送包阶段的输入/回执/步骤（纯数据 + 可读名）
└── submit_sequencer.hpp/.cxx       # 送包时序状态机（读到什么 → 下一步做什么）

backends/mediacodec/session/        # 只在 Android 编译
├── decode_pipeline_gate.hpp/.cxx   # 停止标志 + 输入空位条件变量 + 送包收工标志
├── decode_stage_step.hpp           # 一段一轮的结果（无状态）
├── decode_pipeline_counters.hpp    # 热路径原子计数（纯数据，无 cxx）
├── decode_stage_cost.hpp/.cxx      # 单段平台调用耗时（次数/累计/峰值）
├── decode_failure_slot.hpp/.cxx    # 首个失败原因槽
├── decode_pace_sampler.hpp/.cxx    # 节拍摘要的跨线程发布
├── decode_submit_worker.hpp/.cxx   # 送包线程体：read → submit → 等 room
├── decode_receive_worker.hpp/.cxx  # 收帧线程体：receive → arrive → release
└── decode_session.hpp/.cxx         # 编排者：open/start/stop/snapshot + 模式分支

tests/video/pipeline/p12c6_pipeline_tests/
└── submit_sequencer_tests.cpp      # C6-1 … C6-9（42 断言）
```

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `submit_sequencer` | 由"读到什么/送出去没有"推出下一步 | 不读文件、不碰解码器 |
| `decode_pipeline_gate` | 停止标志 + 输入空位信号 | 不持有解码器、不碰帧数据 |
| `decode_pipeline_counters` | 原子计数 | 不做分位数统计 |
| `decode_stage_cost` | 记一段平台调用的次数/累计/峰值 | 不跨段汇总 |
| `decode_failure_slot` | 记住第一个失败原因 | 不决定要不要停 |
| `decode_pace_sampler` | 锁着发布节拍摘要 | 不改节拍器 |
| `decode_submit_worker` | 送包循环 | **不碰** receive / release |
| `decode_receive_worker` | 收帧 + 节拍 + 释放 | **不碰** extractor / submit |
| `decode_session` | 生命周期与模式编排 | 不含任何循环体 |

> **默认模式是 `kMerged`，不是 `kSplit`。** P12-C6 的真机 A/B 判据未成立
> （增益 1080p8 −0.9% / 1080p10 +2.0% / 4K +0.5%，全在噪声内），按设计 §七 回退。
> 判据推导、瓶颈归因与"P12-B 的 5.2ms 固定开销说法有误"这一修正，见 `20-p12c6-status.md`。
>
> **`core/pipeline/` 不许出现 `<media/*>` / `<android/*>`**：iOS 的 VideoToolbox 同样是
> "送样本 / 收帧"两段，这套时序状态机要能原样搬过去。`grep -rn "android/\|media/" core/pipeline/` 应为空。

---

### 三之六、音视频合成族：`video_plane/backends/avsync/`（P12-C8）

```
backends/avsync/
├── session/
│   ├── audio_playback.hpp/.cxx    # 音频播放侧：解封装 → 解码 → 环 → AAudio → 时钟
│   └── av_sync_stats.hpp          # 合成统计（纯数据，无 cxx）
└── report/
    └── av_sync_report.hpp/.cxx    # kivo.p12c.sync/1

tools/
└── verify_p12c_sync.py            # 独立复算：从分布重放分位数，逐位比对
```

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `audio_playback` | 把一条音轨跑到出声，并交出音频主时钟 | 不认识 Surface、不认识节拍器 |
| `av_sync_stats` | 把两侧事实装成一个结构 | 不挑字段、不算派生值 |
| `av_sync_report` | 纯函数渲染 `kivo.p12c.sync/1` | 不读时钟、不写文件 |
| `verify_p12c_sync.py` | 只吃 JSON 复算分位数与自洽性 | 不 import 任何项目代码 |

**为什么必须有这一层**：`mediacodec` 不该知道出声，`aaudio` 不该知道解码器。
把"两个后端一起跑"塞给任意一边，依赖方向立刻变成双向。
`avsync` 是唯一被允许同时 include 两棵子树的目录，它自己则**不许** include JNI 或 `<jni.h>`。

**同时把 `audio_decode_feeder` 从 `probe/audio/` 提升到
`backends/mediacodec/session/`**：C3/C4 的探针是"用完可以删"的东西，
后端核心一旦依赖它，探针就再也删不掉了。它本就是"解封装 + 音频解码 + 环形缓冲"的组合，
按职责该在 session 层。搬过去只改命名空间与 include，行为零变化。

> **`av_drift` 只在 `cpu_paced` 路径上等于音画偏差。**
> `surface_at_time`（默认）提前 3 帧交出，那个差值等于提交提前量（实测 p50 = −99.875 ms），
> 证据层据此写 `measurable: false` + 理由。推导与实测见 `25-p12c8-status.md` §三。
> **不许**为了让报表好看而把这条判定去掉。

---

### 三之七、Surface 重建（P12-D1）

**没有新模块。** 这个能力属于"解码会话怎么处理输出目标"，拆出去会得到一个
只有两个函数、却要反过来持有 `DecodeSession` 的空壳。按职责归位在 `session/`：

| 文件 | 加了什么 | 为什么放这儿 |
| --- | --- | --- |
| `session/surface_rebuild_tracker.hpp/.cxx` | **新文件**：换面的时刻跟踪（相位机 + 原子量） | 与 `DecodePipelineCounters`（每秒读好几次的热路径计数）语义不同：这个一年走两次。混进去会让两类东西共用一套读法 |
| `decode/mediacodec_video_decoder.hpp/.cxx` | `set_output_surface()` + `surface_mutex_` | 换面是 codec 的能力，不是会话的 |
| `session/decode_session.hpp/.cxx` | `rebind_window()`、`surface_detached_` | 会话决定"没有面的时候怎么办"，这是编排职责 |
| `session/decode_receive_worker.cxx` | 摘面期间强制 `render=false` | 收帧段才知道这一帧有没有地方可上 |

**三条不可越界的约束**（都是真机踩出来的，见 `27-p12d1-status.md` §三）：

1. **不许调 `AMediaCodec_setOutputSurface(codec, nullptr)`。** K40 上切过去就切不回来
   （`AMEDIA_ERROR_UNKNOWN`）。"摘面"必须是本地标志。
2. **摘面期间不许放掉 `ANativeWindow*`。** codec 还指着它，release 完就是悬空指针。
   旧窗口留到新面挂上之后。
3. **空窗丢弃的帧不许并进 `frames_dropped_late`。** 那是"节拍判它太晚"，
   这个不是。并了之后一次切后台看起来就像节拍崩了。

> `surface_mutex_` 只护"换面"与"交帧"这一对，**不给 `submit`/`receive` 加锁** ——
> 它们不碰窗口，加锁只是让热路径更慢。这条边界别随手扩大。

---

### 三之八、多实例并发（P12-D2）

**native 侧没有任何新文件。** 3 路并发 = 三个独立 `SessionHandle`，
每个有自己的 `DecodeSession`、自己的线程、自己的 codec。C6 之后会话本就不共享状态，
所以并发是天然的 —— 这反过来是一条架构约束：

> **任何形式"全局唯一的解码状态"都会让并发退化成串行。**
> 加字段前先问一句：多实例同时跑时它该有几份？该有几份就必须有几份。

新增的只有壳层与校验脚本：

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `android/.../bridge/PlaybackSession.kt` | 一条会话的生命周期包装（handle + fd + 开/启/写/放） | 不碰 UI、不做轮询 |
| `android/.../ui/MultiPlayerActivity.kt` | N 个 `SurfaceView` 横排 + N 条会话，**先全开再全启** | 不验换面（D1 已验）、不碰音频、不 seek |
| `tools/verify_p12d_multi.py` | 从证据算吞吐比与零丢帧，外加帧数闭合与轮内极差 | 不 import 任何项目代码 |

**为什么 `MultiPlayerActivity` 不复用 `PlayerActivity`**：后者的
`onSurfaceAvailable` / `onSurfaceLost` 是**一对一**假设（D1 刚把换面做对），
塞进 N 路会立刻失效。代价是 `PlaybackSession.kt` 与 `PlayerActivity` 的内联逻辑重复 ——
已知且刻意，因为 `PlayerActivity` 承载 C5–C8 与 D1 的全部取证口径，
在 D2 里顺手重构会让那些已收口的复跑命令失效。**收敛归 D4**。

---

### 三之九、长稳采样（P12-D3）

**没有新模块，只有采样开关与两个主机侧脚本。**

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `ui/PlayerActivity.kt`（+`kivo.soak` / `kivo.soak.ms`） | 每 N 秒把当前统计写成一份完整 `decode/6` | 不做内存采样（应用看不到全貌） |
| `tools/sample_p12d3_mem.py` | 主机侧定期 `dumpsys meminfo`，写 CSV | 不判断好坏，只采数 |
| `tools/verify_p12d3_soak.py` | 读 CSV + 快照，判内存增长/丢帧趋势/帧数闭合 | 不 import 项目代码 |

**为什么内存只能在主机侧采**：应用内只看得到自己的 `mallinfo` / `Runtime`，
看不到驱动、SurfaceFlinger、codec 替它占的那部分。真出泄漏时应用内读数往往纹丝不动。

**三条口径写死在脚本里，不许绕过**：

1. **时间窗必须裁两次**：一次按播放区间（设备时钟与主机时钟差 8 小时，必须换算），
   一次按"停机那一跳"（PSS 从 105 掉到 60 MB，那不是长稳表现）。
2. **中途快照不是静止态**：`released == received` 只对结束证据成立；
   中途只查 `0 ≤ 在途 ≤ 16`。
3. **判据是 5 MB 绝对值**，不是比例 —— 比例会让"进程越大允许漏越多"。

> `generated_at_unix_ms` 是**设备时钟**，不是真实时间。这台 K40 比 UTC 快 8 小时。
> 拿它排序没问题，跨机器算先后就会错 —— 这条对 C 组、D1、D2 的证据同样成立。

---

### 三之十、会话收尾顺序（P12-D4）

**一个新头文件，纯数据 + 一个模板执行器，没有 `.cxx`。**

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `session/session_teardown_plan.hpp` | 定义七步、销毁计划、停止前缀、执行器 | 不碰任何资源（那七个函数体在 JNI 侧） |

为什么在 `session/` 而不是新开一族：这七步讲的全是
`DecodeSession` / `AudioPlayback` / `ANativeWindow` / `AudioClockSource` 谁先死，
正是 `session/` 已经在编排的那批。**一个新头文件开新目录必然是个空壳**，
违反"子域文件数 ≥ 3 才开子目录"。

```
kSessionDestroyPlan  = 停视频 → 取音频位置 → 停音频 → 放会话 → 放音频 → 放窗口 → 放时钟
kSessionStopPlanSize = 3           // 停止 = 严格前缀
run_session_teardown(plan, hooks)  // 唯一执行器；JNI 只提供七个函数体
```

**三条不可越界的约束**（改顺序之前先读 `31-p12d4-design.md` §二）：

1. **所有 `stop` 必须早于所有 `reset`。** 反过来的顺序会让线程摸到已释放对象。
2. **停止不许另写一份列表。** 它必须是销毁计划的前缀 —— 两份列表可以各自演化，
   "顺序唯一"就只剩口号。
3. **`kSampleAudioPosition` 必须夹在 `kStopVideo` 与 `kStopAudio` 之间。**
   停在后面读到的是停流后的回退档，会**静默**污染 `timeline.audio_position_at_video_stop_ns`。

> `SessionHandle` 的**成员声明顺序**是这份计划的兜底（析构是声明逆序）。
> 单测测不到 C++ 成员顺序 —— 这条只能靠注释与"排成员时先看计划表"维持，
> 已写进 `32-p12d4-status.md` §六 第 2 条。

---

### 三之十一、上屏时刻（P12-D5）

**C++ 一行没加。** 这张卡的全部实现是两个主机侧脚本 ——
上屏时刻是**合成器的知识**，不在我们进程里；去进程里加代码追它，本质上是在猜。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `tools/probe_present_timing.py` | 清统计 → 起播 → 按**完整层名** dump → 解析 → 落 JSON | 不改播放器、不判断好坏 |
| `tools/verify_p12d5_present.py` | 从原始行独立重算分位数，与汇总逐位比对 | **不 import 探针**（共用代码只能证明"它跟自己一致"） |

**为什么放 `tools/` 而不是 `backend/`**：它读的是 `dumpsys`，不是我们的内存。
放进 `backend/` 会让人以为它随播放器一起发布 —— 它不会。

#### 三条硬规矩（都踩过）

1. **层名必须带 `#id`。** `SurfaceView[...](BLAST)#115401`。
   少了它 `--latency` 只返回刷新率一行，**不报错、不提示** ——
   C7 就是把这句"格式正确但没有数据"读成了"平台不支持"，然后写进了结题文档。
2. **`actual == INT64_MAX` 是"还没锁存"的哨兵**，不是"迟到 292 年"。
   混进 Δ 会把 max 直接毁掉（第一次跑报出 `9222946887098 ms`）。
3. **"稳态"必须按时间顺序切头几帧**，不能 `sorted()` 之后再切 ——
   那削掉的是最小的几个 Δ，等于把下尾砍了，数字会往差里偏。

> `cpu_paced` 下第一列是媒体 pts，与 `actual` 的单调时钟不同源，**Δ 不存在**。
> 工具显式拒绝并写理由 —— 那个模式本来就没有"请求上屏时刻"，不是测不了。

---

### 三之十二、颜色事实（P12-E1）

**两个纯函数文件 + 一个探针。** 关键取舍：判定与取数分开 ——
取数要 `AMediaCodec*`（只能在设备上跑），判定只吃整数（可以在桌面上单测）。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `decode/color_format_names.{hpp,cxx}` | 整数 → 名字 / 位深；是否厂商私有区 | 不碰 `AMediaFormat`，不做判定 |
| `decode/video_color_facts.{hpp,cxx}` | `VideoColorFacts` + `assess_color()`（verdict + reason） | 不读平台、不写 JSON |
| `probe/color_probe_main.cxx` | 设备端可执行 `kivo_color_probe`：三次 configure 问 codec 能不能吐 P010 | 不证明播放路径（产物 `scope` 字段写死这句） |

放 `decode/` 而不是 `session/`：位深是**解码器输出**的属性，
会话只是把它抄进证据。

#### 三条硬规矩（都是实测教训）

1. **`getOutputFormat` 必须在 `start()` 之后读。** configure 后读到的是默认值 21，
   拿它当协商结果会把"还没协商"写成"协商成 NV12"。
2. **"configure 成功"不能当格式支持的证据。** K40 上连
   `color-format = Surface` 且不给窗口都能成功 —— 组件根本不校验这个键。
3. **未知一律写 `null`，不写 0/−1。** `0` 会被读成"值就是 0"，
   `null` 才表示"没有这个事实"。`verdict = indeterminate` 必须带 reason。

> NDK 不导出 `COLOR_Format*` 常量，数值是手写的 —— 由 `kivo_p12e1_color_tests` 钉住。
> 写错不会有编译错误，只会让日志和证据说瞎话。

---

### 三之十三、颜色标记下发（P12-E2）

**一个"值"、一个"事实"、一层平台、一个探针。** 分层依据同 E1：能单测的必须不碰平台。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `decode/color_key_names.hpp` | `color-*` 键的**取值表与名字**（数值来自 `javap android.jar`） | 不读格式、不判定 |
| `decode/hdr_kind.hpp` | `HdrKind` 枚举 + 名字 | 不判定"这片源是什么"（那是 `classify_hdr`） |
| `decode/hdr_delivery_facts.hpp` | `DataSpaceOutcome` + `HdrDeliveryFacts`（纯事实） | 不设置 dataspace（那是 `present/`） |
| `decode/source_color_tags.hpp` | 轨道格式里的颜色标记（纯数据）+ `any()` | 不碰 `AMediaFormat` |
| `decode/mediaformat_color_io.{hpp,cxx}` | **唯一**读写 `AMediaFormat` 颜色键的一层；"复制再写"副本 | 不解释取值的含义 |
| `decode/video_color_facts.{hpp,cxx}` | 源/下发/输出三段事实 + `classify_hdr()` | 不读平台、不写 JSON |
| `present/surface_dataspace.{hpp,cxx}` | 按 `HdrKind` 选 `ADATASPACE_*` 并设到窗口 | **不声称"合成器认了"**（那是 E3） |
| `probe/hdr_probe_main.cxx` | 设备端 `kivo_hdr_probe`：A/B 两次 configure，比 start 后的颜色键 | 不给窗口，不证明播放路径 |

**为什么 `HdrKind` / `DataSpaceOutcome` 从 `present/` 挪到 `decode/`**：它们的产生者是颜色判定，
`present/` 只是消费者。留在 `present/` 会让 `decode/` 反向依赖上屏层 —— 与 C2/C4 把
环形缓冲、时钟守卫提到 `core/` 是同一条理由。

#### 四条硬规矩（都是实测教训）

1. **`color-standard` 上没有 5，也没有 7。** `BT601_NTSC=4`、`BT2020=6`；
   `BT2020_CONSTANT_LUMINANCE` 是 `DataSpace` 的常量，不是这条键的取值。
2. **颜色键来自 SPS VUI，不是容器 `colr` 盒。** 自造片源的 `colr/mdcv/clli` 计数全 0，
   仍读到 `6/6/2`。想补静态元数据，别去解析 MP4 盒子。
3. **`ANativeWindow_setBuffersDataSpace` 在 `libnativewindow.so`。**
   `dlsym(RTLD_DEFAULT)` 拿不到（工程没链接它）；先 `dlopen` 该库再查。
   失败时 reason 必须写清"试过哪个库"。
4. **源没有的键不补默认值。** 补了等于替片源撒谎；缺就缺，证据里写 `null` + reason。

---

### 三之十四、色域标记取证（P12-E3）

**C++ 只加了一个回读函数，判据在合成器侧。** 与 D5 同一条取舍：上屏/合成器的知识不在我们进程里，
去进程里加代码追它，本质上是在猜。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `tools/probe_layer_dataspace.py` | 现取带 `#id` 的 BLAST 层 → 截层块 → 读 `dataspace` / `hdr metadata types` / `dynamic_range` | 不改播放器、不判断好坏 |
| `tools/verify_p12e3_layer.py` | 从 `raw_lines` 用**自己的一套正则**重算，再与字段比对；两组必须相异 | **不 import 探针**（共用代码只能证明"它跟自己一致"） |
| `present/surface_dataspace.cxx` | 新增 `read_surface_dataspace()`（`ANativeWindow_getBuffersDataSpace`，API 29，`dlopen("libnativewindow.so")`） | 不声称"合成器认了" |

#### 三条硬规矩

1. **`HDR Panel:true` 不是状态。** SDR 播放时同样是 true —— 那是面板能力。用它当判据会永远通过。
2. **SF 报的数值 ≠ 我们设的数值**（298188800 vs 163971072，差 range 位）。
   比名字（`BT2020_ITU_PQ`），比整数会得到"下发失败"的假结论。
3. **层名匹配少一个片段就静默拿到空。** `(BLAST)` 与 `#id` 缺一不可；探针匹配不到时显式失败
   并打印候选层名 —— 这是 D5 那次"静默返回空被读成不支持"的直接修正。

---

### 三之十五、tone mapping 的事实与取证（P12-E4）

**没有新增渲染路径。** 本卡交付的是"三条路都试过"的证据 + 一个永远为 `false` 的事实字段。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `decode/hdr_delivery_facts.hpp`（`TonemapFacts`） | 记录"是否实现 / 为什么没做" | 不做映射，不带开关 |
| `decode/mediaformat_color_io.cxx` | 新增 `make_configure_format_with_transfer_request()`：写 `color-transfer-request` | 不判断组件是否采纳（那是探针的事） |
| `probe/hdr_probe_main.cxx` | 第三个用例 + `sdr_transfer_request` 结论块（`hdr-probe/2`） | 不给窗口，不证明 Surface 路径 |
| `tools/analyze_frame_luma.py` | 截图 → Rec.709 亮度中位数/分位/截断比例 | 不算尼特、不做 PSNR/SSIM |

#### 三条硬规矩

1. **`implemented` 恒为 `false`，并且必须带 reason。** 没有这个字段，读者只能从颜色字段里猜
   "到底映射没映射"；有它，答案写在证据里。
2. **"设了但没生效"的开关要单独记。** `set-user-disabled-hdr-types` 在 MIUI 上设完即被清回 ——
   这不是我们命令写错，但也**不是**平台不支持，只是"本机造不出该场景"。
3. **对比截图只能是"同像素、改标记"。** 用 `-c copy -bsf:v hevc_metadata=...` 改 VUI，
   不要重编码 —— 重编码会改像素，"标记的影响"和"编码的影响"就分不开了。

---

### 三之十六、seek 预算与起跑（P12-F1）

**一个新纯族 + 一次函数抽取。** 关键取舍：判定进 `core/`，操作留在 `session/`。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `core/seek/seek_budget.{hpp,cxx}` | `SeekFacts` + `assess_seek()`（总量 vs 预算 + reason） | 不测时间、不碰平台 |
| `session/decode_session.cxx`（`seek_to` / `begin_run`） | 停线程 → 定位 → flush → 起跑 | 不解释判定（那是 `assess_seek`） |
| `source/mediaextractor_source.cxx` | `seek_to` 返回**实际落点** | 不做重试 |
| JNI `nativeSessionSeek` + `PlaybackSession.seekTo()` | 控制线程入口 | 不在工作线程调用 |

#### 三条硬规矩

1. **`begin_run()` 必须被 `start()` 与 `seek_to()` 共用。** 手写两遍就会出现"seek 后少一次
   `pacer_->reset()`"，而首帧时延这个数字看起来仍然正常 —— 没人会发现。
2. **flush 必须在起线程之前。** 反过来新送的包会被 flush 吃掉，表现为"seek 后前几帧没了"，不报错。
3. **`landing` 离 target 远先查关键帧间隔。** `PREVIOUS_SYNC` 落在目标前的同步帧上，
   片源只有开头一个关键帧时它会落回 0（段内帧数会等于整段）。证据里两者都记，就是为了能分辨。

---

### 三之十七、音频 seek 与时钟重锚（P12-F2 / F2A，**已达成**）

**这一节先后经历了两版**：F2 失败（停流 → 时钟失效），F2A 用「静音栅栏」修好。
失败记录留在 `46-p12f2-status.md`，本节只留最终形态与踩过的坑。

| 文件 | 唯一职责 | 状态 |
| --- | --- | --- |
| `core/audio/clock/audio_clock_source.hpp` | 加 `rebase(media_ns)`，默认空实现 | 可用 |
| `core/audio/clock/frame_position_extrapolator.{hpp,cxx}` | `rebase()` 重设帧位置↔媒体时间基准 | 可用 |
| `backends/aaudio/clock/aaudio_clock_source.{hpp,cxx}` | `rebase()` = 外推器 rebase + **守卫 reset** | 可用 |
| `backends/avsync/session/audio_playback.{hpp,cxx}` | `seek_to()`：栅栏 → 停喂 → 定位 → flush → 清环 → 重灌 → 放音 → 重锚（**不 restart 流**） | 可用（14/14 轮零丢帧） |
| `core/seek/seek_budget.{hpp,cxx}` | `assess_av_seek()`：两侧落点差 ≤ 40 ms | 可用（只判落点，判不了"跳完能不能播"） |

#### 四条硬规矩

1. **守卫必须与外推基准一起重置。** `MonotonicClockGuard` 的职责是"不许倒退"，而 seek 恰恰要倒退；
   只改一个，时钟会卡在 seek 前的位置不动，而所有计数看起来正常。
2. **"stale 为假"不等于"时钟活着"。** 停/启流之后第一次读到的往往不是 stale，但时间戳其实没回来；
   判活要判"位置在前进"。
3. **即使判到"在前进"也不够。** 实测时钟在重启后约 **1.7 s** 才成片失效（`fallback_frames ≈ 50`），
   所以"再等一会儿"这条路是死的。这是 F2 BLOCKED 的直接原因。
4. **落点一致 ≠ 同步。** 失败轮 `lateness.p50 = 0`，只有 `clock_mix` 与 `catch_up_frames` 能看出问题。
   任何新增的 seek 判据都必须同时看这两个字段。

#### F2A 追加的两条（比上面四条更硬的教训）

5. **不要为了清环去停流。** 停流会让 `getTimestamp` 成片失效约 1.7 s，节拍器随之切自生成时间轴，
   回来时整段判迟到 —— 这才是 F2 那 2/5 轮 3 秒追赶的真因。**用栅栏，不用停流。**
6. **音频要追视频的落点，不是追用户目标。** 视频走 `PREVIOUS_SYNC` 会落到关键帧上；
   音频没有关键帧概念，会落到目标附近。后向 seek 实测差 490 ms ⇒ 直接违反「不脱节」。
   正确编排：视频 `defer_start` 定位 → 读它的落点 → 音频按该落点定位 → 一起起跑。

---

### 三之十八、负向证据（P12-F3）

**几乎没有新增 C++ 家族**：判断大多早就存在，缺的是"可命名、可校验的负向证据"。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `ui/PlayerActivity.kt`（`writeFailureEvidence`） | `kivo.case` → 按用例命名 + `kivo.p12f.negative/1` | 不带 `kivo.case` 时退回旧文件名 |
| `source/mediaextractor_source.cxx` | `0 tracks` 与"有轨但缺该类型"说成两句不同的话 | 不猜"是不是加密" |
| `probe/hdr_probe_main.cxx` | 输出首包 `is_encrypted` / `is_sync` / `size` | 不下结论，只报事实 |
| `tools/verify_p12f3_errors.py` | 四条用例逐条断言 reason 非空且**指向故障类型**；不可验证的报 `NOT_VERIFIED` | 不用"文件缺失"换通过 |

#### 三条硬规矩

1. **"有 reason" 不等于 "说对了"。** `no video track among 0 tracks` 有 reason，
   但会把人引向"换片源"而不是"文件坏了"—— 判据要连**故障类型**一起查。
2. **不可验证就报 `NOT_VERIFIED`，退出码非 0。** 4 条里过 3 条不是"4 条通过"。
3. **不加启发式检测。** 按"帧数明显偏少"判加密会让坏片/VFR 全部误报；宁可留一个已知盲点，
   也不制造一个随时误伤的开关。

---

### 三之十九、加密轨识别（P12-F3B）

**一个新纯族 + 一个新取值层。** 判据换了一个字段，但换的理由比代码重要。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `source/encryption_key_names.hpp` | `crypto-*` 键名与 `crypto_mode_name()` | 不解读语义 |
| `source/encryption_facts.{hpp,cxx}` | `TrackEncryption` + `assess_encryption()`（三态判定） | **不碰 `AMediaFormat`**（所以桌面可测） |
| `source/mediaformat_encryption.{hpp,cxx}` | `read_track_encryption()`，只读 int32 键 | 不判定 |
| `MediaExtractorSource` | open 选定轨道时抄一次加密事实 | 不在 read 时补读 |
| `DecodeSession::open()` | `configure()` **之前**拒绝（`kEncryptedUnsupported=11`） | 不试图配解码器 |
| `tools/inject_pssh.py` | 往 `moov` 注入 `pssh`（造对照样本） | 不做通用 remux |

#### 四条硬规矩

1. **`crypto-mode` 是主判据，不是 `getSampleFlags() & ENCRYPTED`。** 后者在 K40 上对真 CENC
   **恒为 0** —— 只看它会把加密轨当明文播，还 `completed=true`。这是本项目第二次栽在
   "平台有位、但平台不设"上（第一次是 E1 的 `color-format`）。
2. **判定必须三态，不许二态。** `encrypted` / `clear` / **`unknown`**。把"键缺席"写成
   `clear` 会让平台改键名静默变成"可播"；写成拒绝会让所有明文片源全挂
   （明文片源**同样**没有这个键）。
3. **拒绝点是 `open()`，不是 `submit()`。** 加密轨配解码器的产物是"垃圾帧 + 成功状态"，
   比直接拒绝更糟 —— 它骗的是取证，不是用户。
4. **读 int32 键，不读 data 键。** `crypto-key` 的值要 `AMediaFormat_getBuffer`（API 28），
   minSdk = 26 直接调会加载期崩。这个区别在字段名上就写着（`key_declared` 不是 `has_key`）。

---

### 三之二十、证据总收口（P12-F4）

**这一族不产生运行时能力，它决定"上面十九族说的话还算不算数"。**

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `tools/verify_p12_all.py` | 四层校验 + 8 变异自检 + 机读总表 | 不重写任何子校验器的判据 |
| `probe/*_main.cxx` | 输出 `generated_at_unix_ms` | 不伪造时间戳 |
| `tools/analyze_frame_luma.py` | 同上 | 同上 |

#### 四条硬规矩

1. **总收口只调用，不重写。** 层 4 直接跑 6 个既有校验器。重写会制造两套判据，
   而两套判据必然在某天分叉 —— 那时没人知道该信哪一套。
2. **豁免必须显式输出。** 现在有 46 项（16 份无时间戳 + 30 份中途快照），逐条打印。
   靠隐藏换来的"全绿"比红更危险 —— 它让人以为已经查过了。
3. **校验器必须自带变异自检。** 8 个变异全抓到才算合格，否则退出码 2。
   一个从不失败的总校验器等于没有。
4. **重跑证据前先核对源。** 拿同组播放证据的 `source` 段对照（`mime` / 尺寸 / 时长）。
   F4 就栽过一次：用 10bit 源覆盖了 8bit 探针证据，靠对照才发现。
   证据目录没有版本历史，覆盖一次就找不回来。

---

### 三之二十一、帧率能力与 4K 覆盖（P13-A）

**新增一族：能力探测**（回答"这个分辨率能跑多少 fps"），与 P12-A 的清单族分工不同。

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `bridge/MediaFramerateInventory.kt` | 调 `VideoCapabilities` 三个 API，摊平 codec × 分辨率 | 不判定 |
| `ui/FramerateDumpActivity.kt` | 无 UI 入口：采集 → 写文件 → finish | 不需要 Surface / 片源 |
| `device/mediacodec_framerate_capability.{hpp,cxx}` | 解析 + `best_fps_for` 判定 + JSON | 不碰 Android API |

#### 四条硬规矩

1. **`supportedWidths.upper` 不是"支持这个分辨率"。** 它报 8192×8192 是芯片的**绝对尺寸上限**，
   与帧率、与像素吞吐无关。要问帧率必须用 `getSupportedFrameRatesFor(w, h)` —— 它**与分辨率绑定**。
2. **判据取 `min(声明, 可达)`。** 4K 记录里 `max_fps` 恒为 240，`achievable` 只有 63/149。
   取声明值会得出"4K 能跑 240fps"的荒谬结论。这条被 `P13A-5` 钉死。
3. **能力是 per-codec，不是 per-profile。** HEVC 的五个 profile 在 4K 上报同一个 149fps ——
   API 粒度如此，不是采集错。所以**不能拿 8bit 的结论外推 10bit**，必须单独实测。
4. **证据里新增段落前先 `grep` 键名。** P13-A 暴露了一个活了 6 张卡的 bug：
   F3B 在 renderer 里另起了一个 `"source"` 键，JSON 重复键让原有的
   `source.mime/width/height/duration_us` 静默消失。总收口层 1 现在直接查重复键。

---

### 三之二十二、冷启动与 seek 的分离（P13-B）

**没有新增文件，只加了一个计数器的"影子值"和一条判据。**
但这一节要记的三条教训，比任何代码都重要。

| 改动 | 内容 |
| --- | --- |
| `core/seek/seek_budget.{hpp,cxx}` | 新增 `assess_cold_start()` + `kColdStartBudgetUs = 300'000` |
| `session/decode_session.{hpp,cxx}` | 新增 `cold_start_first_frame_us_`，第一次 `seek_to()` 前抓一次 |
| `session/decode_stats_renderer.cxx` | 新增 `cold_start` 段，schema → `decode/13` |

#### 三条硬规矩

1. **一个计数器不许服务两个场景。** `counters.first_frame_latency_us` 同时被冷启动和
   seek 用，而 `begin_run()` 会重置它 —— 结果是一份证据里根本分不出"第一次起跑"和
   "seek 后起跑"。P13-A 因此把 132 ms 的冷启动当成 seek 首帧，得出"4K 超预算"的错误结论。
   **要么分开存，要么在字段名上写清口径。**
2. **余量薄 ≠ 超标。** 132 ms 对 150 ms 是 1.13×，不是超。判据太紧会让红失去信息量
   （分不清退步和噪声），太松又没意义。**先算余量倍数，再决定要不要动阈值。**
3. **阈值必须按已观测最大值定。** P13-B 首版据单轮数据定 200 ms，真机第一轮就 206 ms 红。
   18 轮测出 4K10 分布 122–146 ms、外加一次 206 ms，才定 300 ms。
   **按中位数或单轮定阈值，等于给判据埋一个随机红的雷。**

---

### 三之二十三、音频证据字段与跨帧率可比性（P13-C）

**代码只改了两行赋值，但这两行补的是两个"从来没写过"的字段。**

| 改动 | 内容 |
| --- | --- |
| `avsync/session/audio_playback.cxx` | `open()` 成功写 `stats_.opened`；`start()` 成功写 `stats_.started`；**`stop()` 不再清 `stats_.started`** |
| `tools/verify_p12_all.py` | 跨字段一致性：有音频时钟/漂移样本时，`opened`/`started` 必须为 true |

#### 三条硬规矩

1. **证据字段必须有写入点。** `AvSyncStats` 有 `opened`/`started`，渲染器一直输出它们，
   但 `AudioPlayback` 从不赋值 —— 15 份证据全部写着 `false`，而同份证据里
   `clock_source=audio_timestamp`。**新增字段时，先确认有人写它。**
2. **证据记"发生过什么"，不记"此刻状态"。** `stop()` 后采集，就不能在 stop 里复位
   `stats_.started`，否则"这一轮出过声"这件事被抹掉。内部状态与证据状态要分开。
3. **跨帧率的 `av_drift` p99 不可比。** 它按 present 采样，60fps 的采样密度是 30fps 的两倍，
   尾巴自然更长。**p50 可比、p99 不可比** —— 引用时要带帧率，或者统一采样率（未做）。

---

## 四、JNI 族：`android/app/src/main/cpp/jni/`

```
jni/
├── jni_player_bridge.cxx        # Java NativePlayer 的全部 native 实现
└── jni_string_convert.cxx       # jstring ↔ std::string（唯一转换点）
```

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `jni_player_bridge.cxx` | 参数校验 + 调 C++ 核心 + 异常转译 | **不含任何解码/渲染逻辑** |
| `jni_string_convert.cxx` | 字符串编解码 | 不做其它类型转换 |

**铁律**：JNI 层是薄的。若 `jni_player_bridge.cxx` 超过 300 行，说明逻辑泄漏，必须下沉到 C++ 核心。

---

## 五、测试族：`android/app/src/test/` 与 `backend/tests/android/`

| 层 | 位置 | 可测内容 |
| --- | --- | --- |
| 纯逻辑单测（宿主机跑） | `backend/tests/android/` | 格式选择策略、能力矩阵排序、时间戳换算 |
| 设备集成测试 | `android/app/src/androidTest/` | 真机 configure 成功性、解码出帧数 |

P12-A 只做第一类：**能在 Windows 上跑的单测优先做**，真机验证靠探针 JSON。

### P12-A 单测清单

```
backend/tests/android/
├── capability/
│   └── codec_ranking_tests.cpp       # 解码器优选排序：硬件优先、支持 profile 优先
└── format/
    └── format_negotiation_tests.cpp  # 位深协商策略：10bit 源不得静默落 8bit
```

### P12-C2 单测清单（已实现）

```
backend/tests/video/audio_plane/p12c_ring_buffer_tests/
└── spsc_audio_ring_buffer_tests.cpp   # R1-R9：顺序/环绕/短写/不覆盖/并发 100 万/零分配/立体声
backend/tests/video/audio_plane/p12c4_clock_tests/
└── audio_clock_tests.cpp              # C1-C12：倒退/跳变/采样率切换/外推/陈旧/回退三档
```

> 并发正确性测试**刻意放在主机侧**而不是 androidTest：
> 无设备也能跑，且必须同时在 x86_64 与 arm64 真机上跑——
> x86 的强内存模型会放过缺失的 `acquire/release`，ARM 才不会。

---

## 六、构建注册

`android/app/src/main/cpp/CMakeLists.txt` 定义：

| target | 源 | 说明 |
| --- | --- | --- |
| `kivo_core_audio` | `core/audio/`（P12-C2 起，C4 扩时钟） | 可移植音频核心（环形缓冲 + PCM 转换 + 时钟纯逻辑），**零平台依赖** |
| `kivo_core_pace` | `core/pace/`（P12-C5） | 可移植呈现节拍器（判定/外推/直方图），**零平台依赖**，链 `kivo_core_audio` |
| `kivo_backend_mediacodec` | `backends/mediacodec/` | Android 视频 + 音频解码后端 |
| `kivo_backend_aaudio` | `backends/aaudio/`（C4 扩 clock/） | Android 音频输出后端 + 时钟平台层 |
| `kivo_audio_probe` | `probe/audio/`（正弦入口） | 音频输出链路探针（可执行，非 `.so`） |
| `kivo_audio_file_probe` | `probe/audio/`（文件入口） | 真实音轨探针：解码 → 环 → 出声（可执行） |
| `kivo_player` | `jni/` | 共享库 `libkivo_player.so`（唯一对外 `.so`） |

主机侧（`backend/CMakeLists.txt`）：

| target | 说明 |
| --- | --- |
| `kivo_p12c_core_audio` | 与 `kivo_core_audio` 同一份源，桌面端构建 |
| `kivo_p12c_ring_buffer_tests` | R1–R9，CTest 注册；**刻意不链接任何平台库**，无设备也能跑并发测试 |
| `kivo_p12c_pcm_tests` | P1–P9，PCM 转换；纯函数，但错一位就影响全部样本 |

依赖方向单向：`kivo_player → kivo_backend_* → kivo_core_audio → （无）`。**禁止反向**。

> `kivo_core` 与 `kivo_jni` 这两个旧名字在文档里存在过，但**代码里从来没有过**。
> 以本表为准；在 `android/app/src/main/cpp/CMakeLists.txt` 里 grep 不到的名字就是假的。

链接系统库：`mediandk`、`android`、`log`、`EGL`、`GLESv3`、`aaudio`。

---

## 七、门闸自检

| 门闸 | 状态 | 说明 |
| --- | --- | --- |
| 1. 构思 | ✅ | `07-p12-plan.md` 第一节 |
| 2. 规划设计 | ✅ | `07-p12-plan.md` 全篇 |
| 3. 自然家族树 | ✅ | 本文档 |
| 4. 实现 | ⏳ | P12-A 起 |
| 5. 单文件单职责 | ✅ | 见各职责表 |
| 6. 禁止平铺 | ✅ | 最深 4 层，无根目录散落 |
| 7. core 零平台头 | ⏳ | 待守卫脚本扩展 |

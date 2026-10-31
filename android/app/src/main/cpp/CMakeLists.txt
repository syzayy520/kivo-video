cmake_minimum_required(VERSION 3.22.1)

project(kivo_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# android/app/src/main/cpp -> 仓库根
set(KIVO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
set(KIVO_BACKEND "${KIVO_ROOT}/backend")

if(NOT EXISTS "${KIVO_BACKEND}/src/video/video_plane")
    message(FATAL_ERROR "KIVO_ROOT resolved incorrectly: ${KIVO_ROOT}")
endif()

# ---------------------------------------------------------------------------
# kivo_core_pace —— 可移植呈现节拍器（P12-C5）
# 零平台依赖（只用 steady_clock + int64 纳秒），桌面 MSVC 与 arm64 编同一份源。
# 与 kivo_core_audio 一样刻意不放 backends/：iOS / 桌面要共用同一套节拍逻辑。
# ---------------------------------------------------------------------------
add_library(kivo_core_pace STATIC
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_action.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_policy.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_histogram.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_stats.cxx"
    # P13-E：分位数的采样口径词汇（三个线格式名字 + "能不能跨帧率比"）。
    # 报告层靠它写 `quantiles.<mode>`，漏注册同样只在 arm64 链接时炸。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/sampling_mode.cxx"
    # P13-D：按媒体时间抽稀的序列 + 漂移趋势（纯计算，零平台依赖）。
    # 这两个文件被 `pace_stats.cxx` 直接调用：漏注册不会在桌面暴露
    # （桌面走 backend/CMakeLists.txt 的另一份清单），只会在链接 arm64 时炸
    # `undefined symbol: DriftSeries::note` —— 即"桌面全绿、APK 编不过"。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_drift_series.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_drift_trend.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/present_gap.cxx"

    # P14-A：三把以前没有的尺子（G2/G3/G4）。
    # `alloc_probe_hook.cxx` 替换全局 `operator new` —— 它必须**直接**出现在这里，
    # 不能藏在静态库里：静态库的目标只有在"符号被需要"时才被拉进来，
    # 而这条尺子的价值恰恰是"每个分配都经过它"。
    "${KIVO_BACKEND}/src/video/video_plane/core/metrics/cpu_time_facts.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/metrics/alloc_probe.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/metrics/alloc_probe_hook.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/metrics/buffer_lease_facts.cxx"
    # P13-I：窗口分布。同样由 `pace_stats.cxx` 直接调用 ——
    # 漏注册的报应一模一样：桌面全绿，arm64 链接时 `undefined symbol: summarize_window`。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_drift_window.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_window_stats.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_hac.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_halves_agreement.cxx"
    # P14-L：派生量的可测性。由 `pace_stats.cxx` / `frame_pacer.cxx` 共用 ——
    # 漏注册的报应与前几张卡一模一样：桌面全绿，arm64 链接时
    # `undefined symbol: measured_missing`。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_measurability.cxx"
    # P13-P：最差帧的前后邻帧。同样由 `pace_stats.cxx` 直接调用 ——
    # 漏注册的老报应：桌面全绿，arm64 链接时 `undefined symbol: WorstEpisodeRecorder::note`。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_worst_episode.cxx"
    # P13-L：累积漂移 + 额度判读。由 `pace_drift_trend.cxx` 调用 ——
    # 漏注册的报应与前几张卡一模一样：桌面全绿，arm64 链接时
    # `undefined symbol: accumulate_drift`。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_drift_budget.cxx"
    # P14-U：钟领先量的逐帧轨迹 + 视频相对墙的偏移。
    # 两份清单（这里与 backend/CMakeLists.txt）都要注册 ——
    # 漏一处的报应是本项目反复踩到的那个：桌面全绿，arm64 链接时
    # `undefined symbol: LeadTraceRecorder::note`。
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_lead_trace.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_wall_offset.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_gate_scale.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/pace_now.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/clock_position_tracker.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pace/frame_pacer.cxx"
)

target_include_directories(kivo_core_pace
    PUBLIC "${KIVO_BACKEND}/include_private"
)

target_compile_options(kivo_core_pace PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

target_link_libraries(kivo_core_pace PUBLIC kivo_core_audio)

# ---------------------------------------------------------------------------
# kivo_core_pipeline —— 可移植流水线时序（P12-C6）
# 送包段的状态机：只认 int64 pts 与"送出去没有"，零平台依赖。
# iOS 的 VideoToolbox 同样是「送样本 / 收帧」两段，这套时序要能直接搬过去。
# ---------------------------------------------------------------------------
add_library(kivo_core_pipeline STATIC
    "${KIVO_BACKEND}/src/video/video_plane/core/pipeline/submit_types.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/pipeline/submit_sequencer.cxx"
    # P12-F1：seek 判定（纯逻辑，与 pipeline 同属"核心零平台依赖"）
    "${KIVO_BACKEND}/src/video/video_plane/core/seek/seek_budget.cxx"
)

target_include_directories(kivo_core_pipeline
    PUBLIC "${KIVO_BACKEND}/include_private"
)

target_compile_options(kivo_core_pipeline PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

# ---------------------------------------------------------------------------
# kivo_backend_mediacodec —— Android 硬解后端（设备探测部分）
# 每个子域独立 target，禁止并入大 target（见 08-p12-family-tree.md 第六节）
# ---------------------------------------------------------------------------
add_library(kivo_backend_mediacodec STATIC
    # ---- device：能力证据链路，逐单元单职责 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_wall_clock.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_system_property_reader.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_device_identity.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_json_string_escaper.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_codec_record_parser.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_codec_profile_namer.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_codec_bit_depth.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_codec_implementation_class.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_capability_inventory.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_capability_json_renderer.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_capability_probe.cxx"
    # P13-A：帧率能力（codec × 分辨率），回答"4K60 能不能"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/device/mediacodec_framerate_capability.cxx"

    # ---- source：解封装 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/source/mediaextractor_source.cxx"
    # P12-F3B：轨道加密（判定 + 取值；取值这层碰 AMediaFormat，只在 Android 上编）
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/source/encryption_facts.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/source/mediaformat_encryption.cxx"

    # ---- decode：解码 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/mediacodec_format_guard.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/mediacodec_codec_name_resolver.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/mediacodec_video_decoder.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/color_format_names.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/video_color_facts.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/mediaformat_color_io.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/mediacodec_output_pcm_format.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/decode/mediacodec_audio_decoder.cxx"

    # ---- present：上屏 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/present/surface_window_binder.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/present/surface_dataspace.cxx"

    # ---- session：编排 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/audio_decode_feeder.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_stats_renderer.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_pace_report.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_pace_sampler.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_stage_cost.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_failure_slot.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/surface_rebuild_tracker.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_pipeline_gate.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_submit_worker.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_receive_worker.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/mediacodec/session/decode_session.cxx"
)

target_include_directories(kivo_backend_mediacodec
    PUBLIC "${KIVO_BACKEND}/include_private"
)

target_compile_options(kivo_backend_mediacodec PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

# 会话按节拍决定"呈现还是丢弃"，所以它依赖节拍器核心（纯逻辑，零平台）；
# 送包时序状态机同理，归 kivo_core_pipeline。
target_link_libraries(kivo_backend_mediacodec PUBLIC log mediandk android kivo_core_pace kivo_core_pipeline)

# ---------------------------------------------------------------------------
# kivo_backend_aaudio —— Android 音频输出后端（P12-C）
# 与 mediacodec 分开成独立 target：解码归解码，出声归出声，
# 出事时能一眼看出是「没出画面」还是「没出声」。
# ---------------------------------------------------------------------------
add_library(kivo_backend_aaudio STATIC
    # ---- device：开流与协商 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/device/aaudio_format_names.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/device/aaudio_stream_opener.cxx"

    # ---- clock：音频主时钟（P12-C4）----
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/clock/aaudio_timestamp_reader.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/clock/aaudio_clock_source.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/clock/aaudio_clock_trace.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/clock/aaudio_clock_tail.cxx"

    # ---- session：启停与统计 ----
    "${KIVO_BACKEND}/src/video/video_plane/backends/aaudio/session/aaudio_output_session.cxx"
)

target_include_directories(kivo_backend_aaudio
    PUBLIC "${KIVO_BACKEND}/include_private"
)

target_compile_options(kivo_backend_aaudio PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

target_link_libraries(kivo_backend_aaudio PUBLIC log aaudio)

# ---------------------------------------------------------------------------
# kivo_core_audio —— 可移植音频核心（P12-C2）
# 零平台依赖：Android 走 AAudio、iOS 走 AudioUnit、桌面走 WASAPI，
# 三端共用同一个 SPSC 环形缓冲。刻意不放在 backends/ 下，
# 否则 iOS 端要么重复实现一份，要么反向依赖 Android 目录。
# ---------------------------------------------------------------------------
add_library(kivo_core_audio STATIC
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/spsc_audio_ring_buffer.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/pcm_format_converter.cxx"
    # P12-C4 时钟的纯逻辑部分。iOS 的 AudioUnit 走同一套外推与单调守卫，
    # 所以它们必须在这里，而不是 backends/aaudio/ 里。
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/clock/audio_clock_types.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/clock/monotonic_clock_guard.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/clock/frame_position_extrapolator.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/clock/system_clock_source.cxx"
    # P13-AB：`kivo.clock_offset_ms` 的注入点。只有探针传了这个参数才会创建它，
    # 不带参数时生产路径上一个分支都不多（见头文件注释）。
    "${KIVO_BACKEND}/src/video/video_plane/core/audio/clock/clock_offset_source.cxx"
)

target_include_directories(kivo_core_audio
    PUBLIC "${KIVO_BACKEND}/include_private"
)

target_compile_options(kivo_core_audio PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

# ---------------------------------------------------------------------------
# kivo_audio_probe —— P12-C 音频链路探针（可执行）
# 独立进程而非 App 内触发：AAudio 在 shell 进程同样可用，
# 这样验证音频链路不需要改一行 Kotlin，也不会把测试代码混进播放器。
# ---------------------------------------------------------------------------
add_executable(kivo_audio_probe
    "${KIVO_BACKEND}/src/video/video_plane/probe/audio/aaudio_tone_source.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/probe/audio/aaudio_probe_report.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/probe/audio/tone_ring_producer.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/probe/audio/aaudio_tone_main.cxx"
)

target_compile_options(kivo_audio_probe PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

target_link_libraries(kivo_audio_probe PRIVATE
    kivo_backend_aaudio
    kivo_core_audio
    log
    aaudio
)

# ---------------------------------------------------------------------------
# kivo_color_probe —— P12-E1 颜色格式协商探针（可执行）
# 只回答一个问题：这个解码器在 ByteBuffer 模式下接不接受 P010。
# 它**不**证明播放路径（Surface）拿到了 10bit —— 产物里写死了这句话。
# ---------------------------------------------------------------------------
add_executable(kivo_color_probe
    "${KIVO_BACKEND}/src/video/video_plane/probe/color_probe_main.cxx"
)
target_compile_options(kivo_color_probe PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)
target_link_libraries(kivo_color_probe PRIVATE
    kivo_backend_mediacodec
    android
    log
)

# ---------------------------------------------------------------------------
# kivo_hdr_probe —— P12-E2 HDR 标记探针（可执行）
# 回答的是"我们写进 configure 格式的颜色键，平台认不认"：
# A 原样传轨道格式 / B 复制再写颜色键，两次都读 start() 之后的输出格式。
# 不给窗口（ByteBuffer 模式），产物里写死了"不证明 Surface 播放路径"。
# ---------------------------------------------------------------------------
add_executable(kivo_hdr_probe
    "${KIVO_BACKEND}/src/video/video_plane/probe/hdr_probe_main.cxx"
)
target_compile_options(kivo_hdr_probe PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)
target_link_libraries(kivo_hdr_probe PRIVATE
    kivo_backend_mediacodec
    android
    log
)

# ---------------------------------------------------------------------------
# kivo_audio_file_probe —— P12-C3 真实音轨探针（可执行）
# 与 kivo_audio_probe 分开：那个验证的是"输出链路"（正弦），
# 这个验证的是"解码链路"（解封装 → 音频解码 → 转 float → 环 → 出声）。
# 参数语义完全不同，硬塞进一个入口只会让 argv 变成互相排斥的开关堆。
# ---------------------------------------------------------------------------
add_executable(kivo_audio_file_probe
    "${KIVO_BACKEND}/src/video/video_plane/probe/audio/audio_file_probe_report.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/probe/audio/audio_file_probe_main.cxx"
)

target_compile_options(kivo_audio_file_probe PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

target_link_libraries(kivo_audio_file_probe PRIVATE
    kivo_backend_mediacodec
    kivo_backend_aaudio
    kivo_core_audio
    log
    mediandk
    aaudio
)

# ---------------------------------------------------------------------------
# kivo_backend_avsync —— 音视频合成（P12-C8）
# 唯一允许同时认识 mediacodec 与 aaudio 的一层。它不认识 JNI，也不认识 Surface：
# 窗口由 JNI 句柄持有，这里只拿"已经开好的 fd"。
# 单独成 target 而不是塞进 kivo_backend_mediacodec：那个的职责是"解码"，
# 一旦它开始 include aaudio，依赖方向就乱了（解码不该知道出声）。
# ---------------------------------------------------------------------------
add_library(kivo_backend_avsync STATIC
    "${KIVO_BACKEND}/src/video/video_plane/backends/avsync/session/audio_playback.cxx"
    "${KIVO_BACKEND}/src/video/video_plane/backends/avsync/report/av_sync_report.cxx"
)

target_include_directories(kivo_backend_avsync
    PUBLIC "${KIVO_BACKEND}/include_private"
)

target_compile_options(kivo_backend_avsync PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

target_link_libraries(kivo_backend_avsync PUBLIC
    kivo_backend_mediacodec
    kivo_backend_aaudio
    kivo_core_audio
)

# ---------------------------------------------------------------------------
# kivo_player —— JNI 共享库（唯一对外 .so）
# ---------------------------------------------------------------------------
add_library(kivo_player SHARED
    "jni/jni_player_bridge.cxx"
    "jni/jni_session_handle.cxx"
)

target_include_directories(kivo_player PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/jni"
)

target_compile_options(kivo_player PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

target_link_libraries(kivo_player PRIVATE
    kivo_backend_avsync
    kivo_backend_mediacodec
    android
    log
)

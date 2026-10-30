#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_stats.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_open_outcome.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_stream_config.hpp"
#include "video/video_plane/backends/aaudio/session/audio_output_stats.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"
#include "video/video_plane/backends/mediacodec/session/audio_decode_feeder.hpp"

namespace kivo::video::video_plane::probe {

/// 一次「真实音频文件 → 解码 → 环形缓冲 → AAudio」探针运行的完整输入。
///
/// 设备信息由入口采集后传入，渲染函数保持纯函数：
/// 同一份输入在任何时刻都必须产出逐字节一致的 JSON。
struct AudioFileProbeReport {
    std::string device_model;
    std::string device_abi;
    std::int32_t device_sdk = 0;

    // ── 片源与轨道 ──
    std::string label;              ///< 显示名（不含路径，证据里不落设备路径）
    std::int64_t file_bytes = 0;
    std::string mime;
    std::int32_t declared_sample_rate = 0;
    std::int32_t declared_channel_count = 0;
    std::int64_t container_duration_us = -1;

    // ── 解码 ──
    std::string codec_name;                  ///< 平台实选名，仅证据用
    core::audio::PcmFormat output_format;    ///< 解码器实际输出格式
    bool output_format_diverged = false;     ///< 与容器声明不一致（采样率/声道）
    mediacodec::AudioDecodeFeedStats feed;

    // ── 输出 ──
    aaudio::AaudioStreamConfig config;
    aaudio::AaudioOpenOutcome open;
    aaudio::AaudioOpenOutcome start;
    aaudio::AudioOutputStats stats;
    std::int32_t ring_capacity_frames = 0;
    bool prefill_reached = false;   ///< 起流前是否灌到了目标水位（短音轨可能到不了）

    // ── 时钟（P12-C4）──
    aaudio::AaudioClockStats clock;

    /// 首个有效时间戳的时刻 − `requestStart` 时刻（ms）。
    /// 这是**设备侧读数**给出的启动开销，必须与 C1 墙钟测出的 24–146 ms 对得上；
    /// 对不上说明我对 `framePosition` 语义的理解错了，整个设计要推翻。
    std::int64_t startup_millis = -1;

    std::int64_t clock_start_ns = 0;  ///< 起流后首次读到的时钟位置
    std::int64_t clock_end_ns = 0;    ///< 收尾时的时钟位置

    std::int64_t wall_millis = 0;   ///< 实际运行时长
    std::string failure;            ///< 非空表示链路中途失败，message 已含原因
};

/// 纯函数：报告 → JSON 文本（含结尾换行）。
[[nodiscard]] std::string render_audio_file_probe_json(const AudioFileProbeReport& report);

}  // namespace kivo::video::video_plane::probe

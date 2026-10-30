#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/aaudio/device/aaudio_open_outcome.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_stream_config.hpp"
#include "video/video_plane/backends/aaudio/session/audio_output_stats.hpp"

namespace kivo::video::video_plane::probe {

/// 一次 AAudio 探针运行的完整输入。
///
/// 设备信息由入口采集后传入，而不是在这里去读系统属性 ——
/// 渲染函数必须是纯函数，同一份输入在任何时刻都要产出逐字节一致的 JSON。
struct AaudioProbeReport {
    std::string device_model;
    std::string device_abi;
    std::int32_t device_sdk = 0;

    aaudio::AaudioStreamConfig config;
    aaudio::AaudioOpenOutcome open;
    aaudio::AaudioOpenOutcome start;
    aaudio::AudioOutputStats stats;

    std::int64_t requested_millis = 0;
    double tone_frequency_hz = 0.0;

    // ── 供给方式（P12-C2 起） ──
    // 直接连通（source 直连回调）时 mode="direct"，其余字段保持 0；
    // 经环形缓冲供给时 mode="ring"，另带生产侧的计数。
    // 两个模式共用一个 schema 是有意的：同一批证据可以直接横向比较
    // "有没有缓冲层" 对欠载与 xrun 的影响。
    std::string feed_mode = "direct";
    double producer_speed = 0.0;         ///< 1.0 = 按实时速率供给，0.5 = 半速（用于制造欠载）
    std::int32_t ring_capacity_frames = 0;
    std::int64_t frames_pushed = 0;
    std::int64_t producer_short_writes = 0;
};

/// 纯函数：报告 → JSON 文本（含结尾换行）。
[[nodiscard]] std::string render_aaudio_probe_json(const AaudioProbeReport& report);

}  // namespace kivo::video::video_plane::probe

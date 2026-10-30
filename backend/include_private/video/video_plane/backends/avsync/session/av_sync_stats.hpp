#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_stats.hpp"
#include "video/video_plane/backends/aaudio/session/audio_output_stats.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediacodec_audio_decoder.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_device_identity.hpp"
#include "video/video_plane/backends/mediacodec/session/audio_decode_feeder.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stats.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"

namespace kivo::video::avsync {

/// 音频播放侧的事实快照。
///
/// 与 `probe::AudioFileProbeReport` 的关系：**不是同一个东西，也不该合并**。
/// 那个是"一次探针运行的完整输入"，带 `failure`、带入口参数语义；
/// 这个是"一条播放会话里音频侧的实况"，要能嵌进 A/V 合成证据。
/// 把探针结构复用到产品路径上，探针就再也删不掉了。
///
/// 与 `AudioOutputStats` 的分工：那个是 AAudio 会话自己数出来的（回调/欠载/静音），
/// 这里是跨三层的合成（解封装 → 解码 → 出声），字段来源各不相同。
struct AudioPlaybackStats {
    bool opened = false;
    bool started = false;

    // ── 轨道与编解码 ──
    std::string mime;
    std::string codec_name;
    std::int32_t declared_sample_rate = 0;
    std::int32_t declared_channel_count = 0;
    std::int64_t container_duration_us = -1;

    /// 解码器**实际**输出格式。容器声明只是期望值，两者不一致必须显式出现在证据里。
    core::audio::PcmFormat output_format{};
    bool output_format_diverged = false;

    // ── 协商结果 ──
    std::int32_t negotiated_sample_rate = 0;
    std::int32_t negotiated_channel_count = 0;
    std::string negotiated_format;
    std::string sharing_mode;
    std::string performance_mode;
    std::int32_t frames_per_burst = 0;

    // ── 缓冲与供给 ──
    std::int32_t ring_capacity_frames = 0;
    bool prefill_reached = false;
    mediacodec::AudioDecodeFeedStats feed{};

    // ── 输出与时钟 ──
    aaudio::AudioOutputStats output{};
    aaudio::AaudioClockStats clock{};

    /// 首个有效时间戳时刻 − `requestStart` 时刻（ms）。设备侧读数，不靠墙钟估。
    std::int64_t startup_millis = -1;
    std::int64_t clock_start_ns = 0;
    std::int64_t clock_end_ns = 0;

    std::string error;
};

/// 一条 A/V 播放会话的完整事实。
///
/// 刻意把 `video` / `audio` / `pace` 三个整结构带进来，而不是在这里挑字段：
/// 挑选是**报表**的职责。在这里挑，等于让"哪些数字进证据"散落在两处，
/// 加一个字段要改两个文件，早晚漏。
struct AvSyncStats {
    std::string source_label;
    std::int64_t source_bytes = 0;
    bool has_video = false;
    bool has_audio = false;

    mediacodec::DecodeStats video{};
    AudioPlaybackStats audio{};

    /// 节拍器完整摘要（含直方图桶）。只在 `stop()` 之后有效 —— 见
    /// `DecodeSession::pace_summary_after_stop()` 的线程约定。
    core::pace::PaceSummary pace{};

    /// 视频会话停下来的那一刻，音频时钟读到哪了（-1 = 没取到）。
    ///
    /// 为什么需要它：音频会话总比视频多活一会儿（收尾、排空、轮询间隔都有开销），
    /// 拿"音频整段跨度"去比"视频 pts 跨度"是拿两段不同长度的窗口相减，
    /// 差值算出来的不是分叉，是收尾耗时。要判断两条时间轴是否同步，
    /// 就必须把两个位置取在同一时刻。
    std::int64_t audio_position_at_video_stop_ns = -1;

    /// 建锚时刻的两侧位置。`av_drift` 是**相对**量，被锚点吸收掉的常量在这里。
    /// 少了这两个数，`av_drift` 会被读成"全链路偏差"。
    std::int64_t anchor_video_pts_ns = 0;
    std::int64_t anchor_audio_position_ns = 0;

    mediacodec::DeviceIdentity device{};
    std::int64_t generated_at_unix_ms = 0;
};

}  // namespace kivo::video::avsync

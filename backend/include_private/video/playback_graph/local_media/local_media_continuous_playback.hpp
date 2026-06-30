#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "video/playback_graph/local_media/local_media_first_frame_pipeline.hpp"

namespace kivo::video::playback_graph::local_media {

struct LocalMediaContinuousStats {
    bool active{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool first_frame_decoded{false};
    bool frame_rendered{false};
    bool continuous_rendering{false};
    std::uint64_t decoded_frame_count{0};
    std::uint64_t rendered_frame_count{0};
    std::int64_t position_ms{0};
    std::int64_t playback_elapsed_ms{0};
    std::uint64_t last_frame_signature{0};
    std::string error;
};

class LocalMediaContinuousPlayback {
public:
    LocalMediaContinuousPlayback();
    ~LocalMediaContinuousPlayback();

    LocalMediaContinuousPlayback(const LocalMediaContinuousPlayback&) = delete;
    LocalMediaContinuousPlayback& operator=(const LocalMediaContinuousPlayback&) = delete;

    [[nodiscard]] bool start(
        const std::string& media_path,
        int video_stream_index,
        const std::string& video_codec) noexcept;
    [[nodiscard]] int pump_packets(int max_packets) noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept { return stats_.active; }
    [[nodiscard]] const LocalMediaContinuousStats& stats() const noexcept { return stats_; }
    [[nodiscard]] const LocalMediaFrameBuffer& frame_buffer() const noexcept {
        return frame_buffer_;
    }

private:
    struct FfmpegSession;

    LocalMediaContinuousStats stats_{};
    LocalMediaFrameBuffer frame_buffer_{};
    std::string media_path_{};
    int video_stream_index_{-1};
    std::string video_codec_{};
    int next_packet_index_{0};
    std::chrono::steady_clock::time_point playback_started_{};
    std::unique_ptr<FfmpegSession> ffmpeg_session_;
};

}  // namespace kivo::video::playback_graph::local_media
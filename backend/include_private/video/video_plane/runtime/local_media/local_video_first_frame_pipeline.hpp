#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

namespace kivo::video::video_plane::runtime::local_media {

struct LocalMediaPipelineStatus {
    static constexpr std::size_t kMaxStreamTracks = 8;

    bool active{false};
    bool source_session_connected{false};
    bool source_opened{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool first_frame_decoded{false};
    bool d3d11_uploaded{false};
    bool frame_rendered{false};
    std::string container;
    std::string source_provider_kind;
    std::string video_codec;
    std::string audio_codec;
    std::int64_t duration_ms{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::int32_t audio_sample_rate{0};
    std::int32_t audio_channels{0};
    std::int32_t video_stream_indices[kMaxStreamTracks]{};
    std::string video_stream_codecs[kMaxStreamTracks]{};
    std::size_t video_stream_count{0};
    std::int32_t audio_stream_indices[kMaxStreamTracks]{};
    std::string audio_stream_codecs[kMaxStreamTracks]{};
    std::size_t audio_stream_count{0};
    std::int32_t subtitle_stream_indices[kMaxStreamTracks]{};
    std::string subtitle_stream_codecs[kMaxStreamTracks]{};
    std::size_t subtitle_stream_count{0};
    std::string error;
};

class LocalMediaFirstFramePipeline {
public:
    [[nodiscard]] bool open(const std::string& media_path) noexcept;
    [[nodiscard]] bool decode_first_frame() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept { return status_.active; }
    [[nodiscard]] const LocalMediaPipelineStatus& status() const noexcept { return status_; }
    [[nodiscard]] const LocalVideoFrameBuffer& frame_buffer() const noexcept {
        return frame_buffer_;
    }
    [[nodiscard]] int video_stream_index() const noexcept { return video_stream_index_; }
    [[nodiscard]] int audio_stream_index() const noexcept { return audio_stream_index_; }

private:
    LocalMediaPipelineStatus status_{};
    LocalVideoFrameBuffer frame_buffer_{};
    std::string media_path_{};
    int video_stream_index_{-1};
    int audio_stream_index_{-1};
};

}  // namespace kivo::video::video_plane::runtime::local_media

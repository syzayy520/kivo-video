#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video::playback_graph::local_media {

struct LocalMediaPipelineStatus {
    bool active{false};
    bool source_opened{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool first_frame_decoded{false};
    bool d3d11_uploaded{false};
    bool frame_rendered{false};
    std::string container;
    std::string video_codec;
    std::int64_t duration_ms{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::string error;
};

struct LocalMediaFrameBuffer {
    bool valid{false};
    std::int32_t width{0};
    std::int32_t height{0};
    std::vector<std::uint8_t> rgba8888;
};

class LocalMediaFirstFramePipeline {
public:
    [[nodiscard]] bool open(const std::string& media_path) noexcept;
    [[nodiscard]] bool decode_first_frame() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept { return status_.active; }
    [[nodiscard]] const LocalMediaPipelineStatus& status() const noexcept { return status_; }
    [[nodiscard]] const LocalMediaFrameBuffer& frame_buffer() const noexcept {
        return frame_buffer_;
    }
    [[nodiscard]] int video_stream_index() const noexcept { return video_stream_index_; }

private:
    LocalMediaPipelineStatus status_{};
    LocalMediaFrameBuffer frame_buffer_{};
    std::string media_path_{};
    int video_stream_index_{-1};
};

}  // namespace kivo::video::playback_graph::local_media
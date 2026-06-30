#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video::playback_graph {

struct LocalMediaPlaybackQuery {
    bool active{false};
    bool source_opened{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool first_frame_decoded{false};
    bool d3d11_uploaded{false};
    bool frame_rendered{false};
    bool frame_pixels_valid{false};
    std::string container;
    std::string video_codec;
    std::string error;
    std::int64_t duration_ms{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::vector<std::uint8_t> rgba8888;
};

}  // namespace kivo::video::playback_graph
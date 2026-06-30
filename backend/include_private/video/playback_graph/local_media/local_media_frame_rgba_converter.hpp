#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine {
struct DecodedFrame;
}

namespace kivo::video::playback_graph::local_media {

struct RgbaFrameBuffer {
    bool valid{false};
    std::int32_t width{0};
    std::int32_t height{0};
    std::vector<std::uint8_t> rgba8888;
    std::string error;
};

[[nodiscard]] RgbaFrameBuffer convert_decoded_frame_to_rgba8888(
    const kivo::cinema_engine::DecodedFrame& frame) noexcept;

}  // namespace kivo::video::playback_graph::local_media
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// DecodedFrame represents a decoded video or audio frame.
// For real playback, frame_data carries the actual pixel/sample bytes.
struct DecodedFrame {
    ContractMetadata metadata;
    std::string frame_id;
    std::string stream_kind;  // "video", "audio"
    int64_t pts{0};
    int64_t dts{0};
    int64_t duration{0};
    int32_t width{0};
    int32_t height{0};
    std::string pixel_format;  // "yuv420p", "nv12", "rgb24", etc.
    int32_t sample_rate{0};
    int32_t channels{0};
    std::string sample_format;  // "s16", "flt", etc.
    bool is_eof{false};
    std::string frame_ref;  // reference to actual frame in adapter

    // Actual pixel/sample data bytes.
    // Video: raw pixel data in pixel_format layout (e.g. YUV420P planar).
    // Audio: raw sample data in sample_format layout (e.g. float planar).
    std::vector<uint8_t> frame_data;

    // Linesize / stride per plane for video frames (0 means tightly packed).
    int32_t linesize[4]{0, 0, 0, 0};
};

}  // namespace kivo::cinema_engine

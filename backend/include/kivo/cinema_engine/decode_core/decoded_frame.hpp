#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// DecodedFrame represents a decoded video or audio frame.
// Actual pixel/sample data is managed by the adapter.
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
};

}  // namespace kivo::cinema_engine

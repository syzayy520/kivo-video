#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct FlipPresenterFirstFrameConfig {
    ContractMetadata metadata;
    std::string config_id;
    bool enable_hdr{false};
    bool enable_zero_copy{true};
    int32_t width{1920};
    int32_t height{1080};
    std::string pixel_format;  // "rgba8", "rgba16f", "nv12"
    bool vsync_enabled{true};
    int32_t max_latency{2};
};

}  // namespace kivo::cinema_engine
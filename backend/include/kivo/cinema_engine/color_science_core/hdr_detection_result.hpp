#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Result of HDR metadata detection from a media source.
struct HdrDetectionResult {
    ContractMetadata metadata;
    std::string detection_id;
    std::string source_id;
    bool is_hdr10{false};
    bool is_hlg{false};
    bool is_dolby_vision{false};
    int64_t hdr10_max_luminance{0};  // nits
    int64_t hdr10_min_luminance{0};  // nits
    std::string color_primaries;     // bt709, bt2020, etc.
};

}  // namespace kivo::cinema_engine

// tone_map_policy.hpp — GATE-019: P5A tone map policy
#pragma once
#include <string>

namespace kivo::video_plane::color {

enum class ToneMapMode {
    None,                                   // no tone mapping
    HdrToSdr,                               // HDR to SDR tone mapping
    HdrToHdr,                               // HDR to HDR tone mapping (display adaptation)
    SdrToHdr,                               // SDR to HDR upmapping
    Auto                                    // automatic mode selection
};

struct ToneMapPolicy {
    ToneMapMode mode{ToneMapMode::None};
    std::string algorithm;                  // tone mapping algorithm name
    uint32_t target_peak_nits{0};           // target peak luminance (nits)
    bool operator==(ToneMapPolicy const&) const = default;
};

}  // namespace kivo::video_plane::color

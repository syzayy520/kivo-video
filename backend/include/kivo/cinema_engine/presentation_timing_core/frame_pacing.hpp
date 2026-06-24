#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// FramePacing controls when frames are presented to the display.
// Handles 23.976/24p/30p/60p content on various refresh rates.
struct FramePacing {
    ContractMetadata metadata;
    std::string pacing_id;
    double content_frame_rate{23.976};
    double display_refresh_rate{60.0};
    std::string pacing_mode;  // "exact_24p", "compatible_60p", "variable"
    double vsync_phase_ms{0.0};
    int64_t target_present_time_us{0};
    bool is_24p_exact{false};
    int64_t frames_presented{0};
    int64_t frames_late{0};
    int64_t frames_dropped{0};
};

}  // namespace kivo::cinema_engine

#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// MasterClock provides the reference clock for AV sync.
// Typically audio-driven for local playback.
struct MasterClock {
    ContractMetadata metadata;
    std::string clock_id;
    std::string clock_source;  // "audio", "video", "external", "adaptive"
    int64_t current_time_us{0};
    double drift_ms{0.0};
    bool is_running{false};
    bool is_synced{false};
};

}  // namespace kivo::cinema_engine

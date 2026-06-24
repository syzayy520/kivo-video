#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// VSyncPolicy defines how vsync is handled for frame presentation.
struct VSyncPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::string vsync_source;  // "dxgi_swap_chain", "dwm", "manual"
    bool vsync_enabled{true};
    bool allow_tearing{false};
    int32_t sync_interval{1};
    double target_latency_ms{16.67};
};

}  // namespace kivo::cinema_engine

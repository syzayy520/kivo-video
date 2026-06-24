#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Bounded telemetry ring buffer for hot-path metrics.
// Capacity-limited to prevent unbounded memory growth.
struct TelemetryRingBuffer {
    ContractMetadata metadata;
    std::string buffer_id;
    int64_t capacity_samples{10000};
    int64_t current_samples{0};
    std::string sample_kind;  // "frame_timing", "audio_drift", "network_throughput"
    bool is_bounded{true};
};

}  // namespace kivo::cinema_engine

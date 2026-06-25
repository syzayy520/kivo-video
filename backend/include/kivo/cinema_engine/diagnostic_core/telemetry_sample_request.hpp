#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request to write a telemetry sample to a ring buffer.
struct TelemetrySampleRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string buffer_id;
    std::string sample_kind;  // "frame_timing", "audio_drift", "network_throughput"
    int64_t timestamp_ns{0};
    double value{0.0};
};

}  // namespace kivo::cinema_engine

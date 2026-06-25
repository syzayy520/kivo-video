#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Result of writing a telemetry sample to a ring buffer.
struct TelemetrySampleResult {
    ContractMetadata metadata;
    std::string result_id;
    std::string request_id;
    bool was_written{false};
    bool was_overwritten{false};
    int64_t current_occupancy{0};
};

}  // namespace kivo::cinema_engine

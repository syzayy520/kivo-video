#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// A single health check operation against a remote source.
// Supports multiple check types for comprehensive source health.
struct SourceHealthCheck {
    ContractMetadata metadata;
    std::string check_id;
    std::string source_id;
    std::string check_type;  // "connectivity", "throughput", "auth", "range_read"
    bool passed{false};
    std::string message;
    int64_t check_timestamp{0};
    int64_t response_time_ms{0};
};

}  // namespace kivo::cinema_engine

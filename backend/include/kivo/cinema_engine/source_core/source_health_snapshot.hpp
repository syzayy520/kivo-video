#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Source health snapshot for monitoring remote source reliability.
// Used by Inspector to display source connection status and throughput.
struct SourceHealthSnapshot {
    ContractMetadata metadata;
    std::string source_id;
    std::string health_status;  // "healthy", "degraded", "unreachable"
    double throughput_estimate{0.0};
    double latency_estimate{0.0};
    std::string last_error;
    int64_t last_check_timestamp{0};
};

}  // namespace kivo::cinema_engine

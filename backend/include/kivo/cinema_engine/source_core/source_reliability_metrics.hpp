#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Reliability statistics for a remote source connection.
// Tracks request success/failure rates and latency.
struct SourceReliabilityMetrics {
    ContractMetadata metadata;
    std::string source_id;
    int64_t total_requests{0};
    int64_t successful_requests{0};
    int64_t failed_requests{0};
    double success_rate{0.0};  // 0.0-1.0
    int consecutive_failures{0};
    int64_t last_success_timestamp{0};
    int64_t average_latency_ms{0};
};

}  // namespace kivo::cinema_engine

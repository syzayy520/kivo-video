#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Health status classification for a remote source.
// Feeds into Inspector for remote source monitoring.
struct SourceHealthStatus {
    ContractMetadata metadata;
    std::string source_id;
    std::string status;  // "healthy", "degraded", "unreachable", "recovering"
    std::string status_reason;
    int64_t status_since_timestamp{0};
};

}  // namespace kivo::cinema_engine

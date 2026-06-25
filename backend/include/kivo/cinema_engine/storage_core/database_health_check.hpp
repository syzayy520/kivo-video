#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Database health check configuration.
// Captures health status, response time, and error tracking.
struct DatabaseHealthCheck {
    ContractMetadata metadata;
    std::string check_id;  // Unique check identifier
    std::string database_type;  // Database type
    bool is_healthy{true};  // Whether database is healthy
    int64_t last_check_ms{0};  // Last check timestamp
    int64_t response_time_ms{0};  // Response time
    int error_count{0};  // Number of consecutive errors
};

}  // namespace kivo::cinema_engine
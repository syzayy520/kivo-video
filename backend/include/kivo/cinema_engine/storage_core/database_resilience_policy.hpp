#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Database resilience policy for backup, recovery, retry, and failover rules.
// Captures resilience configuration for each database type.
struct DatabaseResiliencePolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    std::string database_type;  // Database type (source, playback, decision, cache, telemetry, preference, schema)
    bool backup_enabled{true};  // Whether backup is enabled
    int64_t backup_interval_ms{3600000};  // Backup interval in milliseconds
    bool recovery_enabled{true};  // Whether recovery is enabled
    int max_retry_attempts{3};  // Maximum retry attempts
    int64_t retry_delay_ms{1000};  // Delay between retries
    bool failover_enabled{false};  // Whether failover is enabled
    bool health_check_enabled{true};  // Whether health checks are enabled
};

}  // namespace kivo::cinema_engine
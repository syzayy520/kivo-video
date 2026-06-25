#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Database recovery policy configuration.
// Captures recovery mode, time limits, and data loss tolerance.
struct DatabaseRecoveryPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    std::string database_type;  // Database type
    std::string recovery_mode;  // Recovery mode (point-in-time, last-good, manual)
    int64_t max_recovery_time_ms{300000};  // Maximum recovery time
    std::string data_loss_tolerance;  // Data loss tolerance (none, minimal, moderate)
    bool verification_enabled{true};  // Whether recovery verification is enabled
};

}  // namespace kivo::cinema_engine
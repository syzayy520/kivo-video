#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Application update policy.
// Defines rules for application updates and rollbacks.
struct UpdatePolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    bool auto_update_enabled{true};  // Whether auto-update is enabled
    bool signature_verification{true};  // Whether to verify update signatures
    bool rollback_enabled{true};  // Whether rollback is supported
    int max_update_size_mb{100};  // Maximum update package size
    bool staged_rollout{false};  // Whether to use staged rollout
};

}  // namespace kivo::cinema_engine
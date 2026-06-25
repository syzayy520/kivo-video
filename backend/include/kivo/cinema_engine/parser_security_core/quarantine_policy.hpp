#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Quarantine policy for suspicious files.
// Defines rules for quarantining and handling suspicious media files.
struct QuarantinePolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    int quarantine_duration_seconds{86400};  // How long to quarantine suspicious files (24h)
    bool auto_delete_quarantined{false};  // Whether to auto-delete quarantined files
    int max_quarantine_size_mb{1024};  // Maximum quarantine storage (1GB)
    bool notify_on_quarantine{true};  // Whether to notify when file is quarantined
    int escalation_threshold{3};  // Number of violations before escalation
};

}  // namespace kivo::cinema_engine
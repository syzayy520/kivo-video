#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Database backup policy configuration.
// Captures backup location, retention, and security settings.
struct DatabaseBackupPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    std::string database_type;  // Database type
    std::string backup_location;  // Backup storage location
    int retention_days{30};  // Backup retention period
    bool compression_enabled{true};  // Whether compression is enabled
    bool encryption_enabled{false};  // Whether encryption is enabled
};

}  // namespace kivo::cinema_engine
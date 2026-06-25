#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Schema version tracking for database schema evolution.
// Captures version information, compatibility status, and change metrics.
struct SchemaVersion {
    ContractMetadata metadata;
    std::string version_id;  // Version identifier (semver)
    std::string schema_name;  // Database schema name
    int64_t created_at{0};  // Creation timestamp
    bool is_compatible{true};  // Whether version is compatible
    int breaking_changes{0};  // Number of breaking changes
    int migration_count{0};  // Number of migrations
};

}  // namespace kivo::cinema_engine
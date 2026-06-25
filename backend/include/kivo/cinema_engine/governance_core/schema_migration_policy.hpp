#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Schema migration policy governing database schema evolution.
// Captures migration strategy and backup requirements for
// forward-only, backward-compatible, or destructive upgrades.
struct SchemaMigrationPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    std::string source_schema_version;  // Current schema version
    std::string target_schema_version;  // Target schema version
    bool allow_breaking_changes{false};  // Whether breaking changes are allowed
    bool require_compatibility_test{true};  // Whether compatibility test is required
    bool rollback_enabled{true};  // Whether rollback is supported
    int64_t migration_timeout_ms{30000};  // Maximum migration time
};

}  // namespace kivo::cinema_engine

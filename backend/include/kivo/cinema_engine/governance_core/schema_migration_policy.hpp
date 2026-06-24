#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Schema migration policy governing database schema evolution.
// Captures migration strategy and backup requirements for
// forward-only, backward-compatible, or destructive upgrades.
struct SchemaMigrationPolicy {
    ContractMetadata metadata;
    std::string schema_name;
    int from_version{0};
    int to_version{0};
    std::string migration_strategy;  // "forward_only", "backward_compatible", "destructive"
    bool requires_backup{true};
};

}  // namespace kivo::cinema_engine

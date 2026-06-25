#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Individual migration step for database schema evolution.
// Captures migration type, reversibility, and resource requirements.
struct MigrationStep {
    ContractMetadata metadata;
    std::string step_id;  // Unique step identifier
    std::string source_version;  // Source schema version
    std::string target_version;  // Target schema version
    std::string migration_type;  // Type of migration (add_column, rename_table, etc.)
    bool is_reversible{true};  // Whether step can be reversed
    int64_t estimated_duration_ms{1000};  // Estimated execution time
    bool requires_downtime{false};  // Whether downtime is required
};

}  // namespace kivo::cinema_engine
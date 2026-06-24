#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// SourceDb is the contract for the source configuration database.
// Actual SQLite implementation stays in storage_core.
struct SourceDb {
    ContractMetadata metadata;
    std::string db_path;
    bool is_open{false};
    bool is_healthy{false};
    std::string corruption_recovery_policy;  // "rebuild", "fail_closed", "truncate"
    int64_t source_count{0};
};

}  // namespace kivo::cinema_engine

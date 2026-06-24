#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// DecisionDb is the contract for the decision ledger database.
struct DecisionDb {
    ContractMetadata metadata;
    std::string db_path;
    bool is_open{false};
    bool is_healthy{false};
    std::string corruption_recovery_policy;
    int64_t ledger_count{0};
    int64_t entry_count{0};
};

}  // namespace kivo::cinema_engine

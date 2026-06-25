#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Policy rules for integration gate evaluation.
struct IntegrationGatePolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::vector<std::string> required_subphases;
    bool allow_conditional{true};
    int max_blocking_items{0};
    bool require_evidence{true};
};

}  // namespace kivo::cinema_engine
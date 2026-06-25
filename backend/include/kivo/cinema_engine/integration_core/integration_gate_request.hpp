#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request to evaluate the full integration gate.
struct IntegrationGateRequest {
    ContractMetadata metadata;
    std::string gate_id;
    std::vector<std::string> subphases_to_evaluate;
    bool strict_mode{false};
    std::string timestamp;
};

}  // namespace kivo::cinema_engine
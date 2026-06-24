#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/policy_core/policy_decision.hpp"

namespace kivo::cinema_engine {

PolicyDecision build_fake_policy_decision(
    const ContractMetadata& metadata,
    std::string decision,
    std::vector<std::string> reasons,
    std::string user_visible_summary);

}  // namespace kivo::cinema_engine

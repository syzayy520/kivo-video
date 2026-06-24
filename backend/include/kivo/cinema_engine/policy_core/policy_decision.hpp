#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct PolicyDecision {
  ContractMetadata metadata;
  std::string policy_decision_id;
  std::string decision;
  std::vector<std::string> reasons;
  std::string user_visible_summary;
};

}  // namespace kivo::cinema_engine


#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct ResourceBudgetResult {
  ContractMetadata metadata;
  std::string resource_budget_id;
  std::string cpu_budget;
  std::string gpu_budget;
  std::string memory_budget;
  std::string network_budget;
  std::vector<std::string> pressure_risks;
  bool playback_allowed{true};
};

}  // namespace kivo::cinema_engine


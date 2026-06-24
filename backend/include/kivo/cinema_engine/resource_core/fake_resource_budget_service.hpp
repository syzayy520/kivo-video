#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/resource_core/resource_budget_result.hpp"

namespace kivo::cinema_engine {

ResourceBudgetResult build_fake_resource_budget_result(
    const ContractMetadata& metadata,
    std::string cpu_budget,
    std::string gpu_budget,
    std::string memory_budget,
    std::string network_budget);

}  // namespace kivo::cinema_engine

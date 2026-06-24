#include "kivo/cinema_engine/resource_core/fake_resource_budget_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

ResourceBudgetResult build_fake_resource_budget_result(
    const ContractMetadata& metadata,
    std::string cpu_budget,
    std::string gpu_budget,
    std::string memory_budget,
    std::string network_budget) {
  ResourceBudgetResult result;
  result.metadata = metadata;
  result.resource_budget_id = "resource.fake.budget.0001";
  result.cpu_budget = std::move(cpu_budget);
  result.gpu_budget = std::move(gpu_budget);
  result.memory_budget = std::move(memory_budget);
  result.network_budget = std::move(network_budget);
  result.pressure_risks = {"cpu.thermal", "memory.fragmentation", "gpu.vram_exhaustion"};
  result.playback_allowed = true;
  return result;
}

}  // namespace kivo::cinema_engine

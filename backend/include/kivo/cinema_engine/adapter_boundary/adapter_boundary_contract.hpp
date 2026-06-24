#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct AdapterBoundaryContract {
  ContractMetadata metadata;
  std::string adapter_boundary_id;
  std::string adapter_type;
  std::vector<std::string> supported_operations;
  std::vector<std::string> required_capabilities;
  bool boundary_valid{true};
};

}  // namespace kivo::cinema_engine

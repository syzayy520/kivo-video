#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct CapabilityJoinResult {
  ContractMetadata metadata;
  std::string join_result_id;
  std::optional<std::string> session_id;
  std::string static_capability_ref;
  std::string device_capability_ref;
  std::string session_capability_ref;
  std::vector<std::string> runtime_observation_refs;
  std::vector<std::string> conflicts;
  std::vector<std::string> missing_capabilities;
  double confidence{0.0};
  std::string expiry;
};

}  // namespace kivo::cinema_engine


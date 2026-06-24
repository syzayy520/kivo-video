#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct FeatureGateSnapshot {
  ContractMetadata metadata;
  std::string feature_gate_snapshot_id;
  std::vector<std::string> enabled_features;
  std::vector<std::string> disabled_features;
  std::vector<std::string> experimental_features;
  std::vector<std::string> compliance_limited_features;
  std::string reason;
  std::string created_at;
};

}  // namespace kivo::cinema_engine


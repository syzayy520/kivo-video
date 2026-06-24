#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/feature_gate_core/feature_gate_snapshot.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

FeatureGateSnapshot build_fake_feature_gate_snapshot(
    const ContractMetadata& metadata,
    std::vector<std::string> enabled_features,
    std::vector<std::string> disabled_features,
    std::vector<std::string> experimental_features);

}  // namespace kivo::cinema_engine

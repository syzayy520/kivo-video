#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Rollout strategy configuration.
// Defines percentage, target groups, and timing for feature rollouts.
struct RolloutStrategy {
    ContractMetadata metadata;
    std::string strategy_id;  // Unique strategy identifier
    std::string strategy_name;  // Human-readable strategy name
    float rollout_percentage{0.0f};  // Percentage rollout (0.0-1.0)
    std::vector<std::string> target_groups;  // Target user groups
    int64_t start_time{0};  // Rollout start time
    int64_t end_time{0};  // Rollout end time
};

}  // namespace kivo::cinema_engine
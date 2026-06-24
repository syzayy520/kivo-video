#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SchedulingPolicyConfig {
    ContractMetadata metadata;
    std::string policy_id;
    std::string task_category{"Playback"};
    int priority_level{2};  // 0-10, higher is better
    bool enable_mmcss{true};
    bool fallback_on_failure{true};
};

}  // namespace kivo::cinema_engine
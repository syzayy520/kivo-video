#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SchedulingPolicyStatus {
    ContractMetadata metadata;
    std::string policy_id;
    bool mmcss_registered{false};
    bool mmcss_available{true};
    int current_priority{0};
    std::string last_error;
};

}  // namespace kivo::cinema_engine
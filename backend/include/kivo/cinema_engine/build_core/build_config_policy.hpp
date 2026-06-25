#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Build configuration policy.
// Defines rules for build type, reproducibility, and constraints.
struct BuildConfigPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    std::string build_type{"Release"};  // Build type (Release, Debug, RelWithDebInfo)
    bool reproducible_build{true};  // Whether build must be reproducible
    bool deterministic_timestamps{true};  // Whether timestamps are deterministic
    bool strip_debug_info{false};  // Whether to strip debug info
    int max_build_time_seconds{600};  // Maximum allowed build time (10 minutes)
};

}  // namespace kivo::cinema_engine
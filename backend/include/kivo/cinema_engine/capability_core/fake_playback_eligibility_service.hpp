#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/capability_core/capability_join_result.hpp"

namespace kivo::cinema_engine {

// Build a fake CapabilityJoinResult for testing.
// Simulates capability join with configurable eligibility.
CapabilityJoinResult build_fake_playback_eligibility(
    const ContractMetadata& metadata,
    std::string static_capability_ref,
    std::string device_capability_ref,
    std::string session_capability_ref,
    std::vector<std::string> runtime_observation_refs,
    std::vector<std::string> conflicts,
    std::vector<std::string> missing_capabilities,
    double confidence);

}  // namespace kivo::cinema_engine

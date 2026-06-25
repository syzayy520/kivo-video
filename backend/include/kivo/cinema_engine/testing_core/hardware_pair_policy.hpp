#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Manual hardware pair policy.
// Defines rules for hardware pairing and fallback strategies.
struct HardwarePairPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    std::string hardware_type;  // Hardware type (GPU, audio, network, storage)
    bool pairing_required{true};  // Whether hardware pairing is required
    bool manual_override_allowed{false};  // Whether manual override is allowed
    bool fallback_to_fake{true};  // Whether to fallback to fake if hardware unavailable
    int timeout_ms{5000};  // Timeout for hardware pairing in milliseconds
};

}  // namespace kivo::cinema_engine
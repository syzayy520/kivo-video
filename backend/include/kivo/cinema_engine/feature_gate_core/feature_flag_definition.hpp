#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Feature flag definition.
// Captures flag type, default value, and ownership.
struct FeatureFlagDefinition {
    ContractMetadata metadata;
    std::string flag_id;  // Unique flag identifier
    std::string flag_name;  // Human-readable flag name
    std::string flag_type;  // Flag type (boolean, percentage, variant)
    std::string default_value;  // Default value when flag is not set
    std::string description;  // Flag description
    std::string owner;  // Flag owner team
};

}  // namespace kivo::cinema_engine
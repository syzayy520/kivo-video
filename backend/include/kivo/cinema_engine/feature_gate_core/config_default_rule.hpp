#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Configuration default rule.
// Defines default values and validation for configuration keys.
struct ConfigDefaultRule {
    ContractMetadata metadata;
    std::string rule_id;  // Unique rule identifier
    std::string config_key;  // Configuration key
    std::string default_value;  // Default value
    std::string validation_regex;  // Validation regex pattern
    std::string fallback_value;  // Fallback value if validation fails
    bool override_allowed{true};  // Whether override is allowed
};

}  // namespace kivo::cinema_engine
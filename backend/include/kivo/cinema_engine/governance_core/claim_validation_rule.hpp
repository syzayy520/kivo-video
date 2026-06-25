#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Validation rule for product claims.
// Defines allowed values, rule types, and severity levels.
struct ClaimValidationRule {
    ContractMetadata metadata;
    std::string rule_id;  // Unique rule identifier
    std::string rule_name;  // Human-readable rule name
    std::string rule_type;  // Rule type (vocabulary, evidence, format, category)
    std::vector<std::string> allowed_values;  // Allowed values for this rule
    std::string severity;  // Rule severity (error, warning, info)
};

}  // namespace kivo::cinema_engine
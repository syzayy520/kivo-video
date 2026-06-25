#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Test reproducibility rule.
// Defines requirements for deterministic and reproducible tests.
struct TestReproducibilityRule {
    ContractMetadata metadata;
    std::string rule_id;  // Unique rule identifier
    std::string rule_name;  // Human-readable rule name
    bool deterministic_required{true};  // Whether deterministic behavior is required
    bool seed_control{true};  // Whether random seed control is required
    bool time_mocking{false};  // Whether time mocking is required
    bool environment_isolation{true};  // Whether environment isolation is required
};

}  // namespace kivo::cinema_engine
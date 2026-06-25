#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Test result for schema compatibility verification.
// Captures compatibility status, test duration, and error/warning counts.
struct CompatibilityTestResult {
    ContractMetadata metadata;
    std::string result_id;  // Unique result identifier
    std::string source_version;  // Source schema version
    std::string target_version;  // Target schema version
    bool is_compatible{true};  // Whether versions are compatible
    int64_t test_duration_ms{0};  // Test execution time
    int errors_found{0};  // Number of compatibility errors
    int warnings_found{0};  // Number of warnings
};

}  // namespace kivo::cinema_engine
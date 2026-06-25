#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Aggregates all readiness checks into a release decision.
struct ReadinessMatrix {
    ContractMetadata metadata;
    std::string matrix_id;
    std::string version;
    int total_checks{0};
    int passed_checks{0};
    int failed_checks{0};
    int warning_checks{0};
    int blocking_failures{0};
    bool release_ready{false};
};

}  // namespace kivo::cinema_engine

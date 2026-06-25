#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Tracks failure rate budgets per session to prevent cascading failures.
struct FailureBudget {
    ContractMetadata metadata;
    std::string budget_id;
    std::string component;
    int64_t window_size_seconds{60};
    int max_failures_per_window{5};
    int current_failures{0};
    bool budget_exhausted{false};
    int64_t recovery_cooldown_ms{30000};
};

}  // namespace kivo::cinema_engine

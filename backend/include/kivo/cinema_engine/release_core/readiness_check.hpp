#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Status of a readiness check.
enum class ReadinessCheckStatus {
    Pass,
    Fail,
    Warning,
    Skipped
};

// Describes a single readiness check evaluation.
struct ReadinessCheck {
    ContractMetadata metadata;
    std::string check_id;
    std::string check_area;
    ReadinessCheckStatus check_status{ReadinessCheckStatus::Pass};
    bool blocking{true};
    std::string evidence;
    std::string last_verified_iso;
};

}  // namespace kivo::cinema_engine

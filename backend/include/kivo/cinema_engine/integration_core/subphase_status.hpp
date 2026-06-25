#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Status of a P2 sub-phase.
enum class SubphaseStatusKind {
    Pass,
    Fail,
    Skipped
};

// Status of a single P2 sub-phase.
struct SubphaseStatus {
    ContractMetadata metadata;
    std::string subphase_id;
    SubphaseStatusKind status{SubphaseStatusKind::Skipped};
    std::string evidence;
    std::string last_verified;
};

}  // namespace kivo::cinema_engine
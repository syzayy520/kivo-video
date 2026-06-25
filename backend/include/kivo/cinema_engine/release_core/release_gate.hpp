#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Final gate decision for release.
enum class GateDecision {
    Ready,
    NotReady,
    ConditionalReady
};

// Final gate decision for release.
struct ReleaseGate {
    ContractMetadata metadata;
    std::string gate_id;
    GateDecision gate_decision{GateDecision::NotReady};
    int blocking_items{0};
    int non_blocking_warnings{0};
    std::string gate_message;
    std::string next_action;
};

}  // namespace kivo::cinema_engine

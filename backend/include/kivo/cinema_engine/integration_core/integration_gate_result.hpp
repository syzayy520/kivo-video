#pragma once

#include <map>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/integration_core/subphase_status.hpp"

namespace kivo::cinema_engine {

// Overall decision of the integration gate.
enum class IntegrationDecision {
    Ready,
    NotReady,
    ConditionalReady
};

// Result of the integration gate evaluation.
struct IntegrationGateResult {
    ContractMetadata metadata;
    std::string gate_id;
    IntegrationDecision overall_decision{IntegrationDecision::NotReady};
    std::map<std::string, SubphaseStatus> subphase_results;
    int blocking_issues{0};
    int warnings{0};
    std::string summary_message;
};

}  // namespace kivo::cinema_engine
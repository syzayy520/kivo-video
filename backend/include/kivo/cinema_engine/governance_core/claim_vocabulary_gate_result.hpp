#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Result of claim vocabulary gate validation.
struct ClaimVocabularyGateResult {
    ContractMetadata metadata;
    std::string result_id;
    std::string request_id;
    bool is_allowed{false};
    std::string matched_pattern;
    std::string enforcement_action;  // "allow", "block", "warn"
    std::string reason;
};

}  // namespace kivo::cinema_engine

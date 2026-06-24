#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Claim vocabulary gate preventing false marketing claims.
// Enforces allowed HDR/audio claim patterns and blocks
// forbidden claim patterns at governance level.
struct ClaimVocabularyGate {
    ContractMetadata metadata;
    std::vector<std::string> allowed_hdr_claims;
    std::vector<std::string> allowed_audio_claims;
    std::vector<std::string> forbidden_claim_patterns;
    std::string enforcement_level;  // "strict", "warn", "audit"
};

}  // namespace kivo::cinema_engine

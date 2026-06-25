#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request to validate a product claim against the vocabulary gate.
struct ClaimVocabularyGateRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string gate_id;
    std::string claim_text;
    std::string claim_category;  // "hdr", "audio", "codec", "format"
};

}  // namespace kivo::cinema_engine

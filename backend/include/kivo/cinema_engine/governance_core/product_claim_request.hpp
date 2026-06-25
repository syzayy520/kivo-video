#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Product claim request for validation.
// Captures claim text, category, and evidence for governance gate.
struct ProductClaimRequest {
    ContractMetadata metadata;
    std::string claim_id;  // Unique claim identifier
    std::string claim_text;  // Product claim text
    std::string claim_category;  // Claim category (performance, quality, feature, compatibility)
    std::vector<std::string> evidence_refs;  // References to supporting evidence
    int64_t submission_timestamp{0};  // When claim was submitted
};

}  // namespace kivo::cinema_engine
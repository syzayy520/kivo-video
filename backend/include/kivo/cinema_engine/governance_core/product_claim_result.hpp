#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Product claim validation result.
// Contains validation outcome, errors, and confidence score.
struct ProductClaimResult {
    ContractMetadata metadata;
    std::string claim_id;  // Claim identifier
    bool is_valid{false};  // Whether claim passes validation
    std::vector<std::string> validation_errors;  // List of validation errors
    float confidence_score{0.0f};  // Confidence in validation (0.0-1.0)
    bool requires_review{false};  // Whether claim requires manual review
};

}  // namespace kivo::cinema_engine
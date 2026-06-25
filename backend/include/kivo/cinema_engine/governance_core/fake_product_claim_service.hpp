#pragma once

#include "kivo/cinema_engine/governance_core/product_claim_request.hpp"
#include "kivo/cinema_engine/governance_core/product_claim_result.hpp"
#include "kivo/cinema_engine/governance_core/claim_validation_rule.hpp"

namespace kivo::cinema_engine {

// Fake product claim service for testing.
// Provides static builder functions for product claim requests, results, and validation rules.
class FakeProductClaimService {
 public:
    // Product claim request builders
    static ProductClaimRequest build_valid_performance_claim();
    static ProductClaimRequest build_invalid_performance_claim();
    static ProductClaimRequest build_valid_feature_claim();
    static ProductClaimRequest build_invalid_feature_claim();
    
    // Validation rule builders
    static ClaimValidationRule build_strict_validation_rule();
    static ClaimValidationRule build_relaxed_validation_rule();
    static ClaimValidationRule build_vocabulary_rule();
    static ClaimValidationRule build_evidence_rule();
    
    // Product claim result builders
    static ProductClaimResult build_approved_claim_result();
    static ProductClaimResult build_rejected_claim_result();
    static ProductClaimResult build_review_required_claim_result();
    static ProductClaimResult build_partial_confidence_claim_result();
};

}  // namespace kivo::cinema_engine
#include "kivo/cinema_engine/governance_core/fake_product_claim_service.hpp"

namespace kivo::cinema_engine {

// Product claim request builders
ProductClaimRequest FakeProductClaimService::build_valid_performance_claim() {
    ProductClaimRequest request;
    request.metadata.schema_version = "1.0.0";
    request.claim_id = "valid-performance-001";
    request.claim_text = "This player delivers 4K HDR playback at 60fps";
    request.claim_category = "performance";
    request.evidence_refs = {"benchmark-001", "test-report-001"};
    request.submission_timestamp = 1719312000000;  // 2024-06-25
    return request;
}

ProductClaimRequest FakeProductClaimService::build_invalid_performance_claim() {
    ProductClaimRequest request;
    request.metadata.schema_version = "1.0.0";
    request.claim_id = "invalid-performance-001";
    request.claim_text = "This player is the fastest in the world";
    request.claim_category = "performance";
    request.evidence_refs = {};
    request.submission_timestamp = 1719312000000;
    return request;
}

ProductClaimRequest FakeProductClaimService::build_valid_feature_claim() {
    ProductClaimRequest request;
    request.metadata.schema_version = "1.0.0";
    request.claim_id = "valid-feature-001";
    request.claim_text = "Supports Dolby Vision and HDR10+";
    request.claim_category = "feature";
    request.evidence_refs = {"feature-spec-001"};
    request.submission_timestamp = 1719312000000;
    return request;
}

ProductClaimRequest FakeProductClaimService::build_invalid_feature_claim() {
    ProductClaimRequest request;
    request.metadata.schema_version = "1.0.0";
    request.claim_id = "invalid-feature-001";
    request.claim_text = "Supports all video formats";
    request.claim_category = "feature";
    request.evidence_refs = {};
    request.submission_timestamp = 1719312000000;
    return request;
}

// Validation rule builders
ClaimValidationRule FakeProductClaimService::build_strict_validation_rule() {
    ClaimValidationRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "strict-validation-001";
    rule.rule_name = "Strict Performance Validation";
    rule.rule_type = "performance";
    rule.allowed_values = {"4K", "HDR", "60fps", "120fps"};
    rule.severity = "error";
    return rule;
}

ClaimValidationRule FakeProductClaimService::build_relaxed_validation_rule() {
    ClaimValidationRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "relaxed-validation-001";
    rule.rule_name = "Relaxed Feature Validation";
    rule.rule_type = "feature";
    rule.allowed_values = {"Dolby Vision", "HDR10+", "HDR10", "HLG"};
    rule.severity = "warning";
    return rule;
}

ClaimValidationRule FakeProductClaimService::build_vocabulary_rule() {
    ClaimValidationRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "vocabulary-rule-001";
    rule.rule_name = "Allowed Vocabulary Rule";
    rule.rule_type = "vocabulary";
    rule.allowed_values = {"supports", "compatible", "optimized", "enhanced"};
    rule.severity = "error";
    return rule;
}

ClaimValidationRule FakeProductClaimService::build_evidence_rule() {
    ClaimValidationRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "evidence-rule-001";
    rule.rule_name = "Evidence Requirement Rule";
    rule.rule_type = "evidence";
    rule.allowed_values = {"benchmark", "test-report", "certification", "audit"};
    rule.severity = "warning";
    return rule;
}

// Product claim result builders
ProductClaimResult FakeProductClaimService::build_approved_claim_result() {
    ProductClaimResult result;
    result.metadata.schema_version = "1.0.0";
    result.claim_id = "valid-performance-001";
    result.is_valid = true;
    result.validation_errors = {};
    result.confidence_score = 0.95f;
    result.requires_review = false;
    return result;
}

ProductClaimResult FakeProductClaimService::build_rejected_claim_result() {
    ProductClaimResult result;
    result.metadata.schema_version = "1.0.0";
    result.claim_id = "invalid-performance-001";
    result.is_valid = false;
    result.validation_errors = {"Unsupported claim category", "Missing evidence"};
    result.confidence_score = 0.1f;
    result.requires_review = false;
    return result;
}

ProductClaimResult FakeProductClaimService::build_review_required_claim_result() {
    ProductClaimResult result;
    result.metadata.schema_version = "1.0.0";
    result.claim_id = "valid-feature-001";
    result.is_valid = true;
    result.validation_errors = {};
    result.confidence_score = 0.7f;
    result.requires_review = true;
    return result;
}

ProductClaimResult FakeProductClaimService::build_partial_confidence_claim_result() {
    ProductClaimResult result;
    result.metadata.schema_version = "1.0.0";
    result.claim_id = "invalid-feature-001";
    result.is_valid = false;
    result.validation_errors = {"Overgeneralized claim", "Unverifiable assertion"};
    result.confidence_score = 0.3f;
    result.requires_review = true;
    return result;
}

}  // namespace kivo::cinema_engine
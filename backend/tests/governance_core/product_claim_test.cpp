#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/governance_core/fake_product_claim_service.hpp"

namespace {

using namespace kivo::cinema_engine;

void test_valid_performance_claim() {
    auto request = FakeProductClaimService::build_valid_performance_claim();
    assert(request.metadata.schema_version == "1.0.0");
    assert(request.claim_id == "valid-performance-001");
    assert(request.claim_text == "This player delivers 4K HDR playback at 60fps");
    assert(request.claim_category == "performance");
    assert(request.evidence_refs.size() == 2);
    assert(request.submission_timestamp == 1719312000000);
}

void test_invalid_performance_claim() {
    auto request = FakeProductClaimService::build_invalid_performance_claim();
    assert(request.metadata.schema_version == "1.0.0");
    assert(request.claim_id == "invalid-performance-001");
    assert(request.claim_text == "This player is the fastest in the world");
    assert(request.claim_category == "performance");
    assert(request.evidence_refs.empty());
    assert(request.submission_timestamp == 1719312000000);
}

void test_valid_feature_claim() {
    auto request = FakeProductClaimService::build_valid_feature_claim();
    assert(request.metadata.schema_version == "1.0.0");
    assert(request.claim_id == "valid-feature-001");
    assert(request.claim_text == "Supports Dolby Vision and HDR10+");
    assert(request.claim_category == "feature");
    assert(request.evidence_refs.size() == 1);
    assert(request.submission_timestamp == 1719312000000);
}

void test_invalid_feature_claim() {
    auto request = FakeProductClaimService::build_invalid_feature_claim();
    assert(request.metadata.schema_version == "1.0.0");
    assert(request.claim_id == "invalid-feature-001");
    assert(request.claim_text == "Supports all video formats");
    assert(request.claim_category == "feature");
    assert(request.evidence_refs.empty());
    assert(request.submission_timestamp == 1719312000000);
}

void test_strict_validation_rule() {
    auto rule = FakeProductClaimService::build_strict_validation_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "strict-validation-001");
    assert(rule.rule_name == "Strict Performance Validation");
    assert(rule.rule_type == "performance");
    assert(rule.allowed_values.size() == 4);
    assert(rule.severity == "error");
}

void test_relaxed_validation_rule() {
    auto rule = FakeProductClaimService::build_relaxed_validation_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "relaxed-validation-001");
    assert(rule.rule_name == "Relaxed Feature Validation");
    assert(rule.rule_type == "feature");
    assert(rule.allowed_values.size() == 4);
    assert(rule.severity == "warning");
}

void test_vocabulary_rule() {
    auto rule = FakeProductClaimService::build_vocabulary_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "vocabulary-rule-001");
    assert(rule.rule_name == "Allowed Vocabulary Rule");
    assert(rule.rule_type == "vocabulary");
    assert(rule.allowed_values.size() == 4);
    assert(rule.severity == "error");
}

void test_evidence_rule() {
    auto rule = FakeProductClaimService::build_evidence_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "evidence-rule-001");
    assert(rule.rule_name == "Evidence Requirement Rule");
    assert(rule.rule_type == "evidence");
    assert(rule.allowed_values.size() == 4);
    assert(rule.severity == "warning");
}

void test_approved_claim_result() {
    auto result = FakeProductClaimService::build_approved_claim_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.claim_id == "valid-performance-001");
    assert(result.is_valid == true);
    assert(result.validation_errors.empty());
    assert(result.confidence_score == 0.95f);
    assert(result.requires_review == false);
}

void test_rejected_claim_result() {
    auto result = FakeProductClaimService::build_rejected_claim_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.claim_id == "invalid-performance-001");
    assert(result.is_valid == false);
    assert(result.validation_errors.size() == 2);
    assert(result.confidence_score == 0.1f);
    assert(result.requires_review == false);
}

void test_review_required_claim_result() {
    auto result = FakeProductClaimService::build_review_required_claim_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.claim_id == "valid-feature-001");
    assert(result.is_valid == true);
    assert(result.validation_errors.empty());
    assert(result.confidence_score == 0.7f);
    assert(result.requires_review == true);
}

void test_partial_confidence_claim_result() {
    auto result = FakeProductClaimService::build_partial_confidence_claim_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.claim_id == "invalid-feature-001");
    assert(result.is_valid == false);
    assert(result.validation_errors.size() == 2);
    assert(result.confidence_score == 0.3f);
    assert(result.requires_review == true);
}

}  // namespace

int main() {
    std::cout << "Running product claim tests..." << std::endl;
    
    test_valid_performance_claim();
    test_invalid_performance_claim();
    test_valid_feature_claim();
    test_invalid_feature_claim();
    test_strict_validation_rule();
    test_relaxed_validation_rule();
    test_vocabulary_rule();
    test_evidence_rule();
    test_approved_claim_result();
    test_rejected_claim_result();
    test_review_required_claim_result();
    test_partial_confidence_claim_result();
    
    std::cout << "All 12 product claim tests passed." << std::endl;
    return 0;
}
#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/governance_core/fake_claim_vocabulary_gate_service.hpp"

using namespace kivo::cinema_engine;

// P2-041: Claim Vocabulary Freeze Gate tests

void test_strict_hdr_gate_construction() {
    auto g = FakeClaimVocabularyGateService::build_strict_hdr_gate();
    assert(g.metadata.schema_version == "1.1");
    assert(g.enforcement_level == "strict");
    assert(g.allowed_hdr_claims.size() == 3);
    assert(!g.forbidden_claim_patterns.empty());
    std::cout << "PASS: test_strict_hdr_gate_construction" << std::endl;
}

void test_hdr10_claim_request() {
    auto r = FakeClaimVocabularyGateService::build_hdr10_request();
    assert(r.request_id == "req-hdr10-001");
    assert(r.claim_category == "hdr");
    assert(!r.claim_text.empty());
    std::cout << "PASS: test_hdr10_claim_request" << std::endl;
}

void test_false_dv_claim_request() {
    auto r = FakeClaimVocabularyGateService::build_false_dv_request();
    assert(r.claim_text.find("Dolby Vision") != std::string::npos);
    assert(r.claim_category == "hdr");
    std::cout << "PASS: test_false_dv_claim_request" << std::endl;
}

void test_allowed_result() {
    auto res = FakeClaimVocabularyGateService::build_allowed_result();
    assert(res.is_allowed == true);
    assert(res.enforcement_action == "allow");
    assert(!res.matched_pattern.empty());
    std::cout << "PASS: test_allowed_result" << std::endl;
}

void test_blocked_result() {
    auto res = FakeClaimVocabularyGateService::build_blocked_result();
    assert(res.is_allowed == false);
    assert(res.enforcement_action == "block");
    assert(res.matched_pattern == "Dolby Vision without RPU");
    std::cout << "PASS: test_blocked_result" << std::endl;
}

void test_warned_result() {
    auto res = FakeClaimVocabularyGateService::build_warned_result();
    assert(res.is_allowed == true);
    assert(res.enforcement_action == "warn");
    assert(!res.reason.empty());
    std::cout << "PASS: test_warned_result" << std::endl;
}

void test_warn_audio_gate() {
    auto g = FakeClaimVocabularyGateService::build_warn_audio_gate();
    assert(g.enforcement_level == "warn");
    assert(g.allowed_audio_claims.size() == 4);
    assert(!g.allowed_audio_claims.empty());
    std::cout << "PASS: test_warn_audio_gate" << std::endl;
}

void test_audit_all_gate() {
    auto g = FakeClaimVocabularyGateService::build_audit_all_gate();
    assert(g.enforcement_level == "audit");
    assert(!g.allowed_hdr_claims.empty());
    assert(!g.allowed_audio_claims.empty());
    assert(!g.forbidden_claim_patterns.empty());
    std::cout << "PASS: test_audit_all_gate" << std::endl;
}

void test_empty_gate() {
    auto g = FakeClaimVocabularyGateService::build_empty_gate();
    assert(g.allowed_hdr_claims.empty());
    assert(g.allowed_audio_claims.empty());
    assert(g.forbidden_claim_patterns.empty());
    assert(g.enforcement_level == "strict");
    std::cout << "PASS: test_empty_gate" << std::endl;
}

void test_atmos_request() {
    auto r = FakeClaimVocabularyGateService::build_atmos_request();
    assert(r.claim_category == "audio");
    assert(r.claim_text.find("Dolby Atmos") != std::string::npos);
    std::cout << "PASS: test_atmos_request" << std::endl;
}

int main() {
    // Gate construction tests
    test_strict_hdr_gate_construction();
    test_warn_audio_gate();
    test_audit_all_gate();
    test_empty_gate();

    // Request tests
    test_hdr10_claim_request();
    test_false_dv_claim_request();
    test_atmos_request();

    // Result tests
    test_allowed_result();
    test_blocked_result();
    test_warned_result();

    std::cout << "All P2-041 Claim Vocabulary Freeze Gate tests passed." << std::endl;
    return 0;
}

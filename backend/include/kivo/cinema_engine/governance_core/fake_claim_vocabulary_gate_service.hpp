#pragma once

#include "kivo/cinema_engine/governance_core/claim_vocabulary_gate.hpp"
#include "kivo/cinema_engine/governance_core/claim_vocabulary_gate_request.hpp"
#include "kivo/cinema_engine/governance_core/claim_vocabulary_gate_result.hpp"

namespace kivo::cinema_engine {

// Fake claim vocabulary gate service for P2-041 testing.
class FakeClaimVocabularyGateService {
public:
    // Gate policy builders
    static ClaimVocabularyGate build_strict_hdr_gate();
    static ClaimVocabularyGate build_warn_audio_gate();
    static ClaimVocabularyGate build_audit_all_gate();
    static ClaimVocabularyGate build_empty_gate();

    // Request builders
    static ClaimVocabularyGateRequest build_hdr10_request();
    static ClaimVocabularyGateRequest build_false_dv_request();
    static ClaimVocabularyGateRequest build_atmos_request();
    static ClaimVocabularyGateRequest build_forbidden_pattern_request();

    // Result builders
    static ClaimVocabularyGateResult build_allowed_result();
    static ClaimVocabularyGateResult build_blocked_result();
    static ClaimVocabularyGateResult build_warned_result();
};

}  // namespace kivo::cinema_engine

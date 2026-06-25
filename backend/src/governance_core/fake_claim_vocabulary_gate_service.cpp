#include "kivo/cinema_engine/governance_core/fake_claim_vocabulary_gate_service.hpp"

namespace kivo::cinema_engine {

// --- Gate policy builders ---

ClaimVocabularyGate FakeClaimVocabularyGateService::build_strict_hdr_gate() {
    ClaimVocabularyGate g;
    g.metadata.schema_version = "1.1";
    g.allowed_hdr_claims = {"HDR10", "HLG", "Dolby Vision"};
    g.allowed_audio_claims = {};
    g.forbidden_claim_patterns = {"Dolby Vision without RPU", "HDR10+ without metadata"};
    g.enforcement_level = "strict";
    return g;
}

ClaimVocabularyGate FakeClaimVocabularyGateService::build_warn_audio_gate() {
    ClaimVocabularyGate g;
    g.metadata.schema_version = "1.1";
    g.allowed_hdr_claims = {};
    g.allowed_audio_claims = {"Dolby Atmos", "DTS:X", "LPCM", "PCM"};
    g.forbidden_claim_patterns = {"Dolby Atmos without object metadata"};
    g.enforcement_level = "warn";
    return g;
}

ClaimVocabularyGate FakeClaimVocabularyGateService::build_audit_all_gate() {
    ClaimVocabularyGate g;
    g.metadata.schema_version = "1.1";
    g.allowed_hdr_claims = {"HDR10", "HLG"};
    g.allowed_audio_claims = {"PCM", "AAC"};
    g.forbidden_claim_patterns = {"Dolby Vision", "Dolby Atmos"};
    g.enforcement_level = "audit";
    return g;
}

ClaimVocabularyGate FakeClaimVocabularyGateService::build_empty_gate() {
    ClaimVocabularyGate g;
    g.metadata.schema_version = "1.1";
    g.enforcement_level = "strict";
    return g;
}

// --- Request builders ---

ClaimVocabularyGateRequest FakeClaimVocabularyGateService::build_hdr10_request() {
    ClaimVocabularyGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-hdr10-001";
    r.gate_id = "gate-hdr-strict";
    r.claim_text = "Supports HDR10 playback";
    r.claim_category = "hdr";
    return r;
}

ClaimVocabularyGateRequest FakeClaimVocabularyGateService::build_false_dv_request() {
    ClaimVocabularyGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-dv-false-001";
    r.gate_id = "gate-hdr-strict";
    r.claim_text = "Dolby Vision without RPU metadata";
    r.claim_category = "hdr";
    return r;
}

ClaimVocabularyGateRequest FakeClaimVocabularyGateService::build_atmos_request() {
    ClaimVocabularyGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-atmos-001";
    r.gate_id = "gate-audio-warn";
    r.claim_text = "Dolby Atmos spatial audio";
    r.claim_category = "audio";
    return r;
}

ClaimVocabularyGateRequest FakeClaimVocabularyGateService::build_forbidden_pattern_request() {
    ClaimVocabularyGateRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-forbidden-001";
    r.gate_id = "gate-audit-all";
    r.claim_text = "True Dolby Vision experience";
    r.claim_category = "hdr";
    return r;
}

// --- Result builders ---

ClaimVocabularyGateResult FakeClaimVocabularyGateService::build_allowed_result() {
    ClaimVocabularyGateResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-allowed-001";
    res.request_id = "req-hdr10-001";
    res.is_allowed = true;
    res.matched_pattern = "HDR10";
    res.enforcement_action = "allow";
    res.reason = "Claim matches allowed HDR10 pattern";
    return res;
}

ClaimVocabularyGateResult FakeClaimVocabularyGateService::build_blocked_result() {
    ClaimVocabularyGateResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-blocked-001";
    res.request_id = "req-dv-false-001";
    res.is_allowed = false;
    res.matched_pattern = "Dolby Vision without RPU";
    res.enforcement_action = "block";
    res.reason = "Claim matches forbidden pattern: Dolby Vision without RPU metadata";
    return res;
}

ClaimVocabularyGateResult FakeClaimVocabularyGateService::build_warned_result() {
    ClaimVocabularyGateResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-warned-001";
    res.request_id = "req-atmos-001";
    res.is_allowed = true;
    res.matched_pattern = "";
    res.enforcement_action = "warn";
    res.reason = "Claim allowed at warn level; logged for audit";
    return res;
}

}  // namespace kivo::cinema_engine

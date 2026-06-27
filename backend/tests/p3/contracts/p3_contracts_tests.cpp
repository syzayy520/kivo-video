#include "kivo/p3/provider_integration/provider_binding.hpp"
#include "kivo/p3/sessions/sessions.hpp"
#include "kivo/p3/security/security_contracts.hpp"
#include "kivo/p3/media_delivery/delivery_contracts.hpp"
#include "kivo/p3/media_probe/probe_contracts.hpp"
#include "kivo/p3/playback_decision/decision_contracts.hpp"
#include "kivo/p3/playback_handoff/handoff_contracts.hpp"
#include "kivo/p3/evidence_contracts/evidence_contracts.hpp"
#include "kivo/p3/unsupported/unsupported_boundaries.hpp"

#include <cassert>
#include <iostream>

static void test_provider_binding()
{
    kivo::p3::provider_integration::ProviderBinding b;
    b.binding_id = "bind-001";
    b.origin.origin_id = "origin-001";
    assert(b.is_valid());
    assert(!b.has_credential());
    b.credential_scope = "scope-001";
    assert(b.has_credential());
    assert(b.schema_version == "p3.provider.binding.v1");
    std::cout << "PASS: provider_binding\n";
}

static void test_provider_metadata_hint()
{
    kivo::p3::provider_integration::ProviderMetadataHint h;
    h.confidence = 0.95;
    assert(h.is_high_confidence());
    h.confidence = 0.5;
    assert(!h.is_high_confidence());
    std::cout << "PASS: provider_metadata_hint\n";
}

static void test_source_session_plan()
{
    kivo::p3::sessions::SourceSessionPlan p;
    p.plan_id = "plan-001";
    p.content_encoding_policy = "identity_only";
    assert(p.is_valid());
    p.content_encoding_policy.clear();
    assert(!p.is_valid());
    std::cout << "PASS: source_session_plan\n";
}

static void test_manifest_snapshot_supersede()
{
    kivo::p3::sessions::ManifestSnapshotSupersede s;
    s.snapshot_version = 1;
    assert(s.has_supersede_policy());
    assert(s.supersede_policy == "keep_current_until_p4_ack");
    std::cout << "PASS: manifest_snapshot_supersede\n";
}

static void test_independent_after_transfer()
{
    kivo::p3::sessions::IndependentAfterTransfer i;
    assert(!i.is_valid());
    i.p4_owns_close = true;
    assert(i.is_valid());
    std::cout << "PASS: independent_after_transfer\n";
}

static void test_network_policy_ssrf()
{
    kivo::p3::security::NetworkPolicy n;
    assert(n.block_private_ips);
    assert(n.block_loopback);
    assert(!n.cross_origin_redirect_allowed);
    assert(n.final_connected_ip_validation);
    std::cout << "PASS: network_policy_ssrf\n";
}

static void test_signed_url_handle()
{
    kivo::p3::security::SignedUrlHandle h;
    h.url_material_ref = "material-001";
    h.redacted_url = "redacted";
    h.expiry_ns = 5000;
    assert(h.has_material());
    assert(!h.is_expired(3000));
    assert(h.is_expired(5000));
    std::cout << "PASS: signed_url_handle\n";
}

static void test_secure_memory_policy()
{
    kivo::p3::security::SecureMemoryPolicy p;
    assert(p.token_state == kivo::p3::security::SecureMemoryState::zeroized_after_use);
    assert(p.is_valid_for_supported_path());
    p.token_state = kivo::p3::security::SecureMemoryState::unsupported_blocking;
    assert(!p.is_valid_for_supported_path());
    std::cout << "PASS: secure_memory_policy\n";
}

static void test_hls_boundary_no_segment_scheduler()
{
    kivo::p3::media_delivery::HlsBoundary h;
    assert(!h.has_segment_scheduler());
    std::cout << "PASS: hls_boundary_no_segment_scheduler\n";
}

static void test_dash_boundary_no_abr()
{
    kivo::p3::media_delivery::DashBoundary d;
    assert(!d.has_abr_execution());
    std::cout << "PASS: dash_boundary_no_abr\n";
}

static void test_drm_mixed_clear()
{
    kivo::p3::media_delivery::DrmMixedClear d;
    d.has_drm_track = true;
    d.has_clear_fallback = true;
    assert(d.is_playable());
    d.has_clear_fallback = false;
    assert(!d.is_playable());
    std::cout << "PASS: drm_mixed_clear\n";
}

static void test_critical_constraints_checklist()
{
    kivo::p3::media_probe::CriticalConstraintsChecklist c;
    assert(!c.all_critical_blocking());
    c.source_readable = true;
    c.range_accessible = true;
    c.auth_valid = true;
    c.known_container = true;
    c.video_path_available = true;
    c.audio_path_available = true;
    c.decoder_available = true;
    c.network_policy_satisfied = true;
    c.redaction_policy_satisfied = true;
    c.resource_budget_satisfied = true;
    c.handoff_ready = true;
    assert(c.all_critical_blocking());
    std::cout << "PASS: critical_constraints_checklist\n";
}

static void test_playback_candidate_set()
{
    kivo::p3::playback_decision::PlaybackCandidateSet s;
    assert(s.is_empty());
    kivo::p3::playback_decision::PlaybackCandidate c;
    c.candidate_id = "cand-001";
    c.kind = kivo::p3::playback_decision::CandidateKind::direct_play;
    s.candidates.push_back(c);
    assert(!s.is_empty());
    assert(s.size() == 1);
    std::cout << "PASS: playback_candidate_set\n";
}

static void test_candidate_ranking_has_reasons()
{
    kivo::p3::playback_decision::CandidateRanking r;
    assert(!r.has_reasons());
    r.reasons.push_back("higher_quality");
    assert(r.has_reasons());
    std::cout << "PASS: candidate_ranking_has_reasons\n";
}

static void test_playback_decision_trace_bidirectional()
{
    kivo::p3::playback_decision::PlaybackDecisionTrace t;
    assert(!t.is_bidirectionally_linked());
    t.execution_card_id = "card-001";
    assert(t.is_bidirectionally_linked());
    std::cout << "PASS: playback_decision_trace_bidirectional\n";
}

static void test_handoff_ownership_three_models()
{
    kivo::p3::playback_handoff::PlaybackHandoffContract h;
    h.ownership = kivo::p3::playback_handoff::HandoffOwnershipModel::transfer_ownership;
    assert(h.ownership == kivo::p3::playback_handoff::HandoffOwnershipModel::transfer_ownership);
    h.ownership = kivo::p3::playback_handoff::HandoffOwnershipModel::borrowed_until_close;
    assert(h.ownership == kivo::p3::playback_handoff::HandoffOwnershipModel::borrowed_until_close);
    h.ownership = kivo::p3::playback_handoff::HandoffOwnershipModel::reopen_by_plan;
    assert(h.ownership == kivo::p3::playback_handoff::HandoffOwnershipModel::reopen_by_plan);
    std::cout << "PASS: handoff_ownership_three_models\n";
}

static void test_unsupported_protocol_candidate()
{
    kivo::p3::unsupported::UnsupportedProtocolCandidate u;
    u.protocol_name = "ftp";
    assert(u.reason == "protocol_out_of_p3_scope");
    assert(u.deferral_stage == "P4_or_later");
    assert(u.is_structured_reason);
    std::cout << "PASS: unsupported_protocol_candidate\n";
}

static void test_evidence_performance_policy()
{
    kivo::p3::evidence_contracts::EvidencePerformancePolicy e;
    assert(e.success_path_sync_max_ns == 5'000'000);
    assert(e.success_path_disk_max_ns == 0);
    assert(e.unbounded_growth_blocked);
    std::cout << "PASS: evidence_performance_policy\n";
}

static void test_golden_diff_policy_no_commit_hash()
{
    kivo::p3::evidence_contracts::GoldenDiffPolicy g;
    assert(g.no_current_commit_hash_in_evidence);
    std::cout << "PASS: golden_diff_policy_no_commit_hash\n";
}

int main()
{
    test_provider_binding();
    test_provider_metadata_hint();
    test_source_session_plan();
    test_manifest_snapshot_supersede();
    test_independent_after_transfer();
    test_network_policy_ssrf();
    test_signed_url_handle();
    test_secure_memory_policy();
    test_hls_boundary_no_segment_scheduler();
    test_dash_boundary_no_abr();
    test_drm_mixed_clear();
    test_critical_constraints_checklist();
    test_playback_candidate_set();
    test_candidate_ranking_has_reasons();
    test_playback_decision_trace_bidirectional();
    test_handoff_ownership_three_models();
    test_unsupported_protocol_candidate();
    test_evidence_performance_policy();
    test_golden_diff_policy_no_commit_hash();
    std::cout << "ALL PASS: p3_contracts_tests (19 tests)\n";
    return 0;
}

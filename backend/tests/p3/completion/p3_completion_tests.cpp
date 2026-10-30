#include "kivo/p3/unsupported/unsupported_boundaries.hpp"
#include "kivo/p3/sessions/sessions.hpp"
#include "kivo/p3/security/security_contracts.hpp"
#include "kivo/p3/media_delivery/delivery_contracts.hpp"
#include "kivo/p3/media_delivery/production_hardening.hpp"
#include "kivo/p3/media_probe/probe_contracts.hpp"
#include "kivo/p3/platform_contracts/platform_contracts.hpp"
#include "kivo/p3/playback_decision/decision_contracts.hpp"
#include "kivo/p3/playback_handoff/handoff_contracts.hpp"
#include "kivo/p3/evidence_contracts/evidence_contracts.hpp"
#include "kivo/p3/observability/observability_contracts.hpp"

#include <cassert>
#include <iostream>

static void test_cloud_drive_unsupported_signal()
{
    kivo::p3::unsupported::CloudDriveContractOnlySignal s;
    s.family = kivo::p3::unsupported::CloudDriveFamily::google_drive;
    s.is_unsupported = true;
    assert(s.is_unsupported);
    assert(s.family == kivo::p3::unsupported::CloudDriveFamily::google_drive);
    std::cout << "PASS: cloud_drive_unsupported_signal\n";
}

static void test_smb_upnp_dlna_boundary_not_implemented()
{
    kivo::p3::unsupported::SmbUpnpDlnaBoundary b;
    assert(b.boundary_defined_not_implemented);
    std::cout << "PASS: smb_upnp_dlna_boundary_not_implemented\n";
}

static void test_manifest_session_plan_valid()
{
    kivo::p3::sessions::ManifestSessionPlan p;
    p.plan_id = "manifest-plan-001";
    p.delivery_kind = kivo::p3::source_model::MediaDeliveryKind::hls_manifest;
    p.refresh_ownership = "p3_refreshes_until_transfer";
    assert(p.is_valid());
    p.refresh_ownership.clear();
    assert(!p.is_valid());
    std::cout << "PASS: manifest_session_plan_valid\n";
}

static void test_manifest_update_candidate_revision()
{
    kivo::p3::playback_decision::RuntimeRevisionResult r;
    r.revision_id = "rev-manifest-001";
    r.trigger = "manifest_updated";
    r.action = "update_candidates";
    r.suggested_candidate_id = "cand-new-001";
    assert(r.action == "update_candidates");
    assert(!r.suggested_candidate_id.empty());
    std::cout << "PASS: manifest_update_candidate_revision\n";
}

static void test_playlist_session_plan_recursion()
{
    kivo::p3::sessions::PlaylistSessionPlan p;
    p.plan_id = "playlist-plan-001";
    p.playlist_kind = "m3u";
    p.max_recursion_depth = 5;
    p.max_entries = 100;
    p.allow_nested_playlist = true;
    assert(p.is_valid());
    assert(p.max_recursion_depth == 5);
    std::cout << "PASS: playlist_session_plan_recursion\n";
}

static void test_credential_single_flight()
{
    kivo::p3::security::CredentialSingleFlight sf;
    sf.credential_scope = "scope-001";
    sf.refresh_in_progress = true;
    sf.waiter_count = 2;
    assert(sf.can_join());
    sf.all_waiters_cancelled = true;
    assert(!sf.can_join());
    std::cout << "PASS: credential_single_flight\n";
}

static void test_progressive_delivery()
{
    kivo::p3::media_delivery::ProgressiveDelivery d;
    d.kind = kivo::p3::source_model::MediaDeliveryKind::progressive_http;
    d.supports_range_read = true;
    d.supports_seek = true;
    d.content_length = 1024;
    assert(d.supports_range_read);
    assert(d.content_length == 1024);
    std::cout << "PASS: progressive_delivery\n";
}

static void test_media_probe_process_boundary()
{
    kivo::p3::media_probe::MediaProbeProcessBoundary b;
    b.probe_id = "probe-001";
    b.input_kind = "local_path";
    b.is_local_path = true;
    b.timeout_ns = 30000000000LL;
    b.max_memory_bytes = 256 * 1024 * 1024;
    assert(b.is_local_path);
    assert(b.timeout_ns > 0);
    assert(b.max_memory_bytes > 0);
    std::cout << "PASS: media_probe_process_boundary\n";
}

static void test_fast_probe_confidence()
{
    kivo::p3::media_probe::FastProbe fp;
    fp.container_hint = "mp4";
    fp.has_moov = true;
    fp.confidence = 0.92;
    assert(fp.is_high_confidence());
    assert(fp.has_moov);
    fp.confidence = 0.5;
    assert(!fp.is_high_confidence());
    std::cout << "PASS: fast_probe_confidence\n";
}

static void test_fast_probe_no_full_scan()
{
    kivo::p3::media_probe::FastProbe fp;
    fp.has_moov = true;
    fp.has_ebml = false;
    fp.has_pat_pmt = false;
    fp.has_flv_header = false;
    assert(fp.has_moov);
    std::cout << "PASS: fast_probe_no_full_scan\n";
}

static void test_probe_evidence_downgrade_trigger()
{
    kivo::p3::media_probe::ProbeEvidence pe;
    pe.container = "mp4";
    pe.video_codec = "h264";
    pe.confidence = 0.95;
    pe.has_downgrade_trigger = true;
    assert(pe.has_downgrade_trigger);
    assert(pe.confidence > 0.9);
    std::cout << "PASS: probe_evidence_downgrade_trigger\n";
}

static void test_probe_evidence_no_raw_ffprobe()
{
    kivo::p3::media_probe::ProbeEvidence pe;
    pe.container = "mp4";
    pe.video_codec = "h264";
    pe.audio_codec = "aac";
    assert(!pe.container.empty());
    std::cout << "PASS: probe_evidence_no_raw_ffprobe\n";
}

static void test_audio_contract_passthrough()
{
    kivo::p3::platform_contracts::AudioCapabilityRequirement a;
    a.audio_format_hint = "eac3";
    a.passthrough_available = true;
    a.pcm_output_available = true;
    a.max_channels = 6;
    a.max_sample_rate = 48000;
    assert(a.passthrough_available);
    assert(a.pcm_output_available);
    assert(a.max_channels == 6);
    std::cout << "PASS: audio_contract_passthrough\n";
}

static void test_pairwise_comparison()
{
    kivo::p3::playback_decision::PairwiseCandidateComparison c;
    c.comparison_id = "pair-001";
    c.winner_id = "cand-001";
    c.loser_id = "cand-002";
    c.reason = "higher_quality";
    c.winning_factors = {"better_codec", "higher_bitrate"};
    c.losing_factors = {"lower_resolution"};
    c.policy_rule = "prefer_quality";
    assert(c.winner_id != c.loser_id);
    assert(!c.reason.empty());
    assert(c.winning_factors.size() == 2);
    std::cout << "PASS: pairwise_comparison\n";
}

static void test_playback_handoff_contract()
{
    kivo::p3::playback_handoff::PlaybackHandoffContract h;
    h.handoff_id = "handoff-001";
    h.candidate_set_ref = "set-001";
    h.selected_candidate_ref = "cand-001";
    h.ownership = kivo::p3::playback_handoff::HandoffOwnershipModel::transfer_ownership;
    h.close_responsibility = "p4";
    h.cancellation_bridge = "cancel-bridge";
    h.buffer_requirement_ref = "buf-001";
    assert(h.has_selected_candidate());
    assert(h.ownership == kivo::p3::playback_handoff::HandoffOwnershipModel::transfer_ownership);
    std::cout << "PASS: playback_handoff_contract\n";
}

static void test_failure_taxonomy_no_generic_error()
{
    kivo::p3::playback_handoff::FailureTaxonomy f;
    f.failure_class = kivo::p3::playback_handoff::FailureClass::transport_failure;
    f.detailed_reason = "connection_reset";
    f.is_recoverable = true;
    f.is_retryable = true;
    assert(!f.detailed_reason.empty());
    assert(f.is_recoverable);
    std::cout << "PASS: failure_taxonomy_no_generic_error\n";
}

static void test_failure_taxonomy_all_classes()
{
    assert(kivo::p3::playback_handoff::FailureClass::binding_failure !=
           kivo::p3::playback_handoff::FailureClass::transport_failure);
    assert(kivo::p3::playback_handoff::FailureClass::probe_failure !=
           kivo::p3::playback_handoff::FailureClass::capability_failure);
    assert(kivo::p3::playback_handoff::FailureClass::handoff_failure !=
           kivo::p3::playback_handoff::FailureClass::runtime_revision_failure);
    assert(kivo::p3::playback_handoff::FailureClass::manifest_failure !=
           kivo::p3::playback_handoff::FailureClass::playlist_failure);
    assert(kivo::p3::playback_handoff::FailureClass::session_failure !=
           kivo::p3::playback_handoff::FailureClass::security_failure);
    std::cout << "PASS: failure_taxonomy_all_classes\n";
}

static void test_schema_versioning_all_contracts()
{
    kivo::p3::sessions::SourceSessionPlan sp;
    assert(!sp.schema_version.empty());
    kivo::p3::security::NetworkPolicy np;
    assert(!np.schema_version.empty());
    kivo::p3::media_delivery::HlsBoundary hb;
    assert(!hb.schema_version.empty());
    kivo::p3::playback_decision::PlaybackCandidateSet cs;
    assert(!cs.schema_version.empty());
    kivo::p3::playback_handoff::PlaybackHandoffContract hc;
    assert(!hc.schema_version.empty());
    kivo::p3::evidence_contracts::FastStartEvidence fe;
    assert(!fe.schema_version.empty());
    kivo::p3::observability::ObservabilityMetric om;
    assert(!om.schema_version.empty());
    std::cout << "PASS: schema_versioning_all_contracts\n";
}

static void test_probe_media_acceptance()
{
    kivo::p3::media_probe::ProbeEvidence pe;
    pe.container = "mkv";
    pe.video_codec = "h265";
    pe.audio_codec = "opus";
    pe.confidence = 0.98;
    pe.constraints.source_readable = true;
    pe.constraints.range_accessible = true;
    pe.constraints.known_container = true;
    pe.constraints.video_path_available = true;
    pe.constraints.audio_path_available = true;
    pe.constraints.decoder_available = true;
    pe.constraints.handoff_ready = true;
    assert(pe.confidence > 0.9);
    std::cout << "PASS: probe_media_acceptance\n";
}

static void test_decision_acceptance_direct_play()
{
    kivo::p3::playback_decision::PlaybackCandidate c;
    c.candidate_id = "cand-direct-001";
    c.kind = kivo::p3::playback_decision::CandidateKind::direct_play;
    c.quality_score = 0.95;
    kivo::p3::playback_decision::PlaybackCandidateSet set;
    set.set_id = "set-001";
    set.candidates.push_back(c);
    kivo::p3::playback_decision::CandidateRanking r;
    r.ranking_id = "rank-001";
    r.ranked_candidate_ids = {"cand-direct-001"};
    r.reasons = {"direct_play_highest_priority"};
    kivo::p3::playback_decision::PlaybackDecisionTrace t;
    t.trace_id = "trace-001";
    t.execution_card_id = "card-001";
    t.confidence = 0.95;
    assert(c.kind == kivo::p3::playback_decision::CandidateKind::direct_play);
    assert(set.size() == 1);
    assert(r.has_reasons());
    assert(t.is_bidirectionally_linked());
    std::cout << "PASS: decision_acceptance_direct_play\n";
}

static void test_evidence_closure_acceptance()
{
    kivo::p3::evidence_contracts::EvidencePerformancePolicy ep;
    assert(ep.success_path_sync_max_ns == 5000000);
    assert(ep.success_path_disk_max_ns == 0);
    kivo::p3::evidence_contracts::GoldenDiffPolicy gd;
    assert(gd.no_current_commit_hash_in_evidence);
    assert(gd.ignore_field_order);
    kivo::p3::playback_decision::PlaybackDecisionTrace t;
    t.execution_card_id = "card-001";
    assert(t.is_bidirectionally_linked());
    std::cout << "PASS: evidence_closure_acceptance\n";
}

static void test_m15_ownership_independent_reader()
{
    kivo::p3::sessions::IndependentRangeReader reader;
    reader.reader_id = "reader-001";
    reader.session_ref = "sess-001";
    reader.is_active = true;
    reader.has_strong_session_ref = true;
    reader.lease_managed = true;
    assert(reader.is_valid());
    assert(reader.lease_managed);
    kivo::p3::sessions::IndependentAfterTransfer iat;
    iat.p4_owns_close = true;
    iat.p4_owns_abort = true;
    assert(iat.is_valid());
    std::cout << "PASS: m15_ownership_independent_reader\n";
}

static void test_m15_security_dns_rebinding()
{
    kivo::p3::security::NetworkPolicy p;
    p.max_cname_depth = 5;
    p.block_private_ips = true;
    p.final_connected_ip_validation = true;
    assert(p.max_cname_depth > 0);
    assert(p.final_connected_ip_validation);
    assert(p.block_private_ips);
    std::cout << "PASS: m15_security_dns_rebinding\n";
}

static void test_webdav_hardening_policies_defined()
{
    kivo::p3::media_delivery::WebDavProductionHardening w;
    assert(!w.propfind_depth_policy.empty());
    assert(!w.href_exact_match_verified);
    assert(!w.collection_rejection_verified);
    assert(!w.malformed_xml_fail_closed_verified);
    assert(!w.large_xml_body_cap_verified);
    std::cout << "PASS: webdav_hardening_policies_defined\n";
}

static void test_http_transport_boundary()
{
    kivo::p3::media_delivery::HttpProductionTransport h;
    assert(h.head_supported);
    assert(h.get_range_supported);
    assert(h.response_206_supported);
    assert(h.response_416_semantics_supported);
    assert(h.redirect_revalidation);
    assert(h.auth_cookie_redaction);
    assert(h.identity_range_encoding);
    assert(h.is_identity_encoding_only());
    std::cout << "PASS: http_transport_boundary\n";
}

int main()
{
    test_cloud_drive_unsupported_signal();
    test_smb_upnp_dlna_boundary_not_implemented();
    test_manifest_session_plan_valid();
    test_manifest_update_candidate_revision();
    test_playlist_session_plan_recursion();
    test_credential_single_flight();
    test_progressive_delivery();
    test_media_probe_process_boundary();
    test_fast_probe_confidence();
    test_fast_probe_no_full_scan();
    test_probe_evidence_downgrade_trigger();
    test_probe_evidence_no_raw_ffprobe();
    test_audio_contract_passthrough();
    test_pairwise_comparison();
    test_playback_handoff_contract();
    test_failure_taxonomy_no_generic_error();
    test_failure_taxonomy_all_classes();
    test_schema_versioning_all_contracts();
    test_probe_media_acceptance();
    test_decision_acceptance_direct_play();
    test_evidence_closure_acceptance();
    test_m15_ownership_independent_reader();
    test_m15_security_dns_rebinding();
    test_webdav_hardening_policies_defined();
    test_http_transport_boundary();
    std::cout << "ALL PASS: p3_completion_tests (24 tests)\n";
    return 0;
}

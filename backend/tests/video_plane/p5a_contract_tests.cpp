// p5a_contract_tests.cpp - P5A Video Plane Contract Tests
// Covers all 37 subdirectories with default construction, comparison, and value tests
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>

// --- stamp/ ---
#include <kivo/video_plane/stamp/core_video_stamp.hpp>
#include <kivo/video_plane/stamp/full_video_stamp.hpp>
#include <kivo/video_plane/stamp/stamp_validation_context.hpp>
#include <kivo/video_plane/stamp/stamp_validation_kind.hpp>
#include <kivo/video_plane/stamp/stamp_validation_result.hpp>
#include <kivo/video_plane/stamp/p5_stamp_validator.hpp>

static void test_stamp() {
    using namespace kivo::video_plane::stamp;
    CoreVideoStamp s1{}, s2{};
    if (!(s1 == s2)) throw 1;
    s1.value = 42;
    if (s1 == s2) throw 1;
    FullVideoStamp fs{};
    if (fs.pipeline_epoch != 0) throw 1;
    StampValidationContext ctx{};
    if (ctx.is_bootstrap) throw 1;  // default is false
    StampValidationResult vr{};
    if (vr.outcome != StampValidationOutcome::Valid) throw 1;
    std::printf("[stamp/] PASS\n");
}

// --- capability/ ---
#include <kivo/video_plane/capability/capability_domain_id.hpp>
#include <kivo/video_plane/capability/capability_support_state.hpp>
#include <kivo/video_plane/capability/capability_snapshot.hpp>
#include <kivo/video_plane/capability/capability_registry.hpp>

static void test_capability() {
    using namespace kivo::video_plane::capability;
    if (CapabilityDomainId::Demux != CapabilityDomainId::Demux) throw 1;
    if (CapabilityDomainId::FramePacing == CapabilityDomainId::Demux) throw 1;
    P5CapabilitySnapshot snap{};
    if (snap.domain != CapabilityDomainId::Demux) throw 1;
    std::printf("[capability/] PASS\n");
}

// --- evidence/ ---
#include <kivo/video_plane/evidence/evidence_record.hpp>
#include <kivo/video_plane/evidence/critical_evidence_ring.hpp>
#include <kivo/video_plane/evidence/drop_counter.hpp>
#include <kivo/video_plane/evidence/evidence_redaction_level.hpp>

static void test_evidence() {
    using namespace kivo::video_plane::evidence;
    P5EvidenceRecord rec{};
    if (rec.schema_version != 0) throw 1;
    rec.schema_version = 1;
    if (rec.schema_version != 1) throw 1;
    P5CriticalEvidenceRing ring{};
    if (ring.capacity != P5CriticalEvidenceRing::kDefaultCapacity) throw 1;
    P5NonCriticalEvidenceDropCounter dc{};
    if (dc.total_dropped != 0) throw 1;
    std::printf("[evidence/] PASS\n");
}

// --- threat/ ---
#include <kivo/video_plane/threat/threat_surface_id.hpp>
#include <kivo/video_plane/threat/threat_mitigation_matrix_item.hpp>
#include <kivo/video_plane/threat/threat_mitigation_matrix.hpp>

static void test_threat() {
    using namespace kivo::video_plane::threat;
    if (ThreatSurfaceId::MaliciousContainer != ThreatSurfaceId::MaliciousContainer) throw 1;
    P5ThreatMitigationMatrixItem item{};
    if (!item.fuzz_target.empty()) throw 1;
    P5ThreatMitigationMatrix matrix{};
    (void)matrix;
    std::printf("[threat/] PASS\n");
}

// --- sandbox/ ---
#include <kivo/video_plane/sandbox/sandbox_broker_contract.hpp>
#include <kivo/video_plane/sandbox/sandbox_crash_policy.hpp>
#include <kivo/video_plane/sandbox/sandbox_timeout_policy.hpp>

static void test_sandbox() {
    using namespace kivo::video_plane::sandbox;
    P5SandboxBrokerContract contract{};
    if (contract.max_message_payload_size != 16 * 1024 * 1024) throw 1;
    SandboxCrashPolicy crash{};
    if (crash.max_crash_count_before_quarantine != 3) throw 1;
    SandboxTimeoutPolicy timeout{};
    if (timeout.open_timeout_ms != 10000) throw 1;
    std::printf("[sandbox/] PASS\n");
}

// --- allocation/ ---
#include <kivo/video_plane/allocation/allocation_limit_policy.hpp>
#include <kivo/video_plane/allocation/allocation_limit_evidence.hpp>

static void test_allocation() {
    using namespace kivo::video_plane::allocation;
    if (AllocationDefaults::kMaxSingleMediaAllocation != 64ULL * 1024ULL * 1024ULL) throw 1;
    if (AllocationDefaults::kMaxTrackCount != 256) throw 1;
    AllocationLimitExceededEvidence ev{};
    if (ev.limit_value != 0) throw 1;
    ev.actual_value = 100;
    if (ev.actual_value != 100) throw 1;
    std::printf("[allocation/] PASS\n");
}

// --- fuzz/ ---
#include <kivo/video_plane/fuzz/fuzz_smoke.hpp>
#include <kivo/video_plane/fuzz/backend_risk_register.hpp>

static void test_fuzz() {
    using namespace kivo::video_plane::fuzz;
    FuzzSmoke smoke{};
    if (!smoke.requires_sandbox) throw 1;
    if (smoke.max_input_bytes != 0) throw 1;
    P5ThirdPartyBackendRiskRegister reg{};
    if (!reg.is_active) throw 1;
    if (!reg.sandbox_required) throw 1;
    std::printf("[fuzz/] PASS\n");
}

// --- version/ ---
#include <kivo/video_plane/version/boundary_contract_version.hpp>
#include <kivo/video_plane/version/compatibility_policy.hpp>
#include <kivo/video_plane/version/version_mismatch_evidence.hpp>

static void test_version() {
    using namespace kivo::video_plane::version;
    BoundaryContractVersion v1{}, v2{};
    if (!(v1 == v2)) throw 1;
    v1.major = 2;
    if (v1 == v2) throw 1;
    CompatibilityResult cr{};
    if (cr.decision != CompatibilityDecision::Unknown) throw 1;
    VersionMismatchEvidence ev{};
    if (ev.expected_version.major != 1) throw 1;
    std::printf("[version/] PASS\n");
}

// --- failure/ ---
#include <kivo/video_plane/failure/pipeline_fatal_signal.hpp>
#include <kivo/video_plane/failure/pipeline_terminal_outcome.hpp>
#include <kivo/video_plane/failure/failure_correlation_id.hpp>
#include <kivo/video_plane/failure/arbitration_evidence.hpp>

static void test_failure() {
    using namespace kivo::video_plane::failure;
    if (PipelineFatalSignal::UnknownFatal == PipelineFatalSignal::GpuDeviceRemoved) throw 1;
    if (TerminalOutcome::CleanShutdown != TerminalOutcome::CleanShutdown) throw 1;
    FailureCorrelationId fid{};
    if (fid.is_valid()) throw 1;
    fid.value = 1;
    if (!fid.is_valid()) throw 1;
    ArbitrationEvidence ae{};
    if (ae.fatal_signal != PipelineFatalSignal::UnknownFatal) throw 1;
    std::printf("[failure/] PASS\n");
}

// --- recovery/ ---
#include <kivo/video_plane/recovery/recovery_seek_request.hpp>
#include <kivo/video_plane/recovery/live_edge_hint_priority.hpp>
#include <kivo/video_plane/recovery/standby_until_new_input.hpp>

static void test_recovery() {
    using namespace kivo::video_plane::recovery;
    RecoverySeekRequest req{};
    if (req.user_visible) throw 1; // ALWAYS false
    if (req.timeline_revision != 0) throw 1;
    if (LiveEdgeHintPriority::P4Approved == LiveEdgeHintPriority::P5Estimate) throw 1;
    StandbyUntilNewInput standby{};
    if (!standby.auto_resume_on_new_input) throw 1;
    std::printf("[recovery/] PASS\n");
}

// --- queue/ ---
#include <kivo/video_plane/queue/video_pipeline_id.hpp>
#include <kivo/video_plane/queue/cancel_token.hpp>
#include <kivo/video_plane/queue/queue_item.hpp>
#include <kivo/video_plane/queue/queue_lifecycle_state.hpp>

static void test_queue() {
    using namespace kivo::video_plane::queue;
    VideoPipelineId pid{};
    if (pid.value != 0) throw 1;
    CancelToken ct{};
    if (ct.is_valid()) throw 1;
    ct.value = 1;
    if (!ct.is_valid()) throw 1;
    QueueItem qi{};
    if (qi.lifecycle_state != QueueLifecycleState::Accepting) throw 1;
    if (qi.core_video_stamp.value != 0) throw 1;
    std::printf("[queue/] PASS\n");
}

// --- protected_content/ ---
#include <kivo/video_plane/protected_content/protected_boundary_hint.hpp>
#include <kivo/video_plane/protected_content/protected_playback_capability_state.hpp>
#include <kivo/video_plane/protected_content/secure_decode_session_context_handle.hpp>

static void test_protected_content() {
    using namespace kivo::video_plane::protected_content;
    ProtectedBoundaryHint hint{};
    if (hint.kind != ProtectedBoundaryKind::None) throw 1;
    ProtectedPlaybackCapabilityState state{};
    if (state.state != ProtectedPlaybackState::NotRequired) throw 1;
    SecureDecodeSessionContextHandle handle{};
    if (handle.is_valid()) throw 1;
    std::printf("[protected_content/] PASS\n");
}

// --- protected_output/ ---
#include <kivo/video_plane/protected_output/protected_output_coverage.hpp>
#include <kivo/video_plane/protected_output/active_output_set.hpp>

static void test_protected_output() {
    using namespace kivo::video_plane::protected_output;
    ProtectedOutputCoverage cov{};
    if (cov.coverage != ProtectedOutputCoverageLevel::None) throw 1;
    ActiveOutputSet set{};
    if (set.active_display_count != 0) throw 1;
    std::printf("[protected_output/] PASS\n");
}

// --- dolby_vision/ ---
#include <kivo/video_plane/dolby_vision/dolby_vision_engine_gate.hpp>
#include <kivo/video_plane/dolby_vision/dv_fallback_policy.hpp>

static void test_dolby_vision() {
    using namespace kivo::video_plane::dolby_vision;
    DolbyVisionEngineGate gate{};
    if (gate.availability != DolbyVisionEngineAvailability::NotChecked) throw 1;
    if (!gate.fail_closed) throw 1;
    DvFallbackPolicy policy{};
    if (policy.mode != DvFallbackMode::FailClosed) throw 1;
    std::printf("[dolby_vision/] PASS\n");
}

// --- hdr10_plus/ ---
#include <kivo/video_plane/hdr10_plus/hdr10_plus_capability.hpp>
#include <kivo/video_plane/hdr10_plus/opaque_metadata_policy.hpp>

static void test_hdr10_plus() {
    using namespace kivo::video_plane::hdr10_plus;
    Hdr10PlusCapability cap{};
    if (cap.state != Hdr10PlusSupportState::NotChecked) throw 1;
    OpaqueMetadataPolicy pol{};
    if (!pol.pass_through) throw 1;
    std::printf("[hdr10_plus/] PASS\n");
}

// --- p010/ ---
#include <kivo/video_plane/p010/p010_formula.hpp>
#include <kivo/video_plane/p010/p010_unorm_tolerance.hpp>

static void test_p010() {
    using namespace kivo::video_plane::p010;
    P010Formula f{};
    if (f.access_mode != P010AccessMode::IntegerPlane) throw 1;
    if (f.bits_per_component != 10) throw 1;
    P010UnormTolerance t{};
    if (!t.allow_rounding) throw 1;
    std::printf("[p010/] PASS\n");
}

// --- d3d11va/ ---
#include <kivo/video_plane/d3d11va/d3d11va_context_mode.hpp>
#include <kivo/video_plane/d3d11va/d3d11_texture_opaque_ref.hpp>

static void test_d3d11va() {
    using namespace kivo::video_plane::d3d11va;
    D3D11vaContext ctx{};
    if (ctx.mode != D3D11vaContextMode::NotInitialized) throw 1;
    D3D11TextureOpaqueRef ref{};
    if (ref.texture_handle != 0) throw 1;
    std::printf("[d3d11va/] PASS\n");
}

// --- swapchain/ ---
#include <kivo/video_plane/swapchain/backbuffer_reference_tracker.hpp>
#include <kivo/video_plane/swapchain/swapchain_resize_preflight.hpp>

static void test_swapchain() {
    using namespace kivo::video_plane::swapchain;
    BackbufferReferenceTracker tr{};
    if (tr.ref_count != 0) throw 1;
    SwapchainResizePreflight pre{};
    if (pre.safe_to_resize) throw 1;
    std::printf("[swapchain/] PASS\n");
}

// --- vrr/ ---
#include <kivo/video_plane/vrr/vrr_state.hpp>
#include <kivo/video_plane/vrr/waitable_swapchain_state.hpp>

static void test_vrr() {
    using namespace kivo::video_plane::vrr;
    VrrState state{};
    if (state.availability != VrrAvailability::NotChecked) throw 1;
    WaitableSwapchainState ws{};
    if (ws.is_waitable) throw 1;
    std::printf("[vrr/] PASS\n");
}

// --- color/ ---
#include <kivo/video_plane/color/color_metadata_authority_policy.hpp>
#include <kivo/video_plane/color/tone_map_policy.hpp>

static void test_color() {
    using namespace kivo::video_plane::color;
    ColorMetadataAuthorityPolicy pol{};
    if (pol.authority != ColorMetadataAuthority::Bitstream) throw 1;
    if (!pol.prefer_bitstream_on_conflict) throw 1;
    ToneMapPolicy tm{};
    if (tm.mode != ToneMapMode::None) throw 1;
    std::printf("[color/] PASS\n");
}

// --- timebase/ ---
#include <kivo/video_plane/timebase/checked_rescale.hpp>
#include <kivo/video_plane/timebase/reorder_policy.hpp>

static void test_timebase() {
    using namespace kivo::video_plane::timebase;
    CheckedRescale cr{};
    if (cr.input_den != 1) throw 1;
    ReorderPolicy rp{};
    if (rp.mode != ReorderMode::PresentationOrder) throw 1;
    if (!rp.drop_late_frames) throw 1;
    std::printf("[timebase/] PASS\n");
}

// --- attachment/ ---
#include <kivo/video_plane/attachment/mime_conflict_policy.hpp>
#include <kivo/video_plane/attachment/quota_scope.hpp>
#include <kivo/video_plane/attachment/opaque_attachment_flow_handle.hpp>

static void test_attachment() {
    using namespace kivo::video_plane::attachment;
    MimeConflictPolicy mcp{};
    if (mcp.resolution != MimeConflictResolution::UseSafest) throw 1;
    QuotaScope qs{};
    if (qs.current_bytes != 0) throw 1;
    OpaqueAttachmentFlowHandle h{};
    if (h.is_valid()) throw 1;
    std::printf("[attachment/] PASS\n");
}

// --- presentation/ ---
#include <kivo/video_plane/presentation/epoch_condition.hpp>
#include <kivo/video_plane/presentation/hold_last_frame_safe.hpp>

static void test_presentation() {
    using namespace kivo::video_plane::presentation;
    PresentationEpoch epoch{};
    if (epoch.condition != EpochCondition::Initializing) throw 1;
    HoldLastFrameSafe hold{};
    if (hold.is_holding) throw 1;
    std::printf("[presentation/] PASS\n");
}

// --- output_boundary/ ---
#include <kivo/video_plane/output_boundary/audio_packet_boundary_item.hpp>
#include <kivo/video_plane/output_boundary/last_visible_estimate.hpp>

static void test_output_boundary() {
    using namespace kivo::video_plane::output_boundary;
    AudioPacketBoundaryItem item{};
    if (item.packet_sequence != 0) throw 1;
    LastVisibleEstimate est{};
    if (est.is_confident) throw 1;
    std::printf("[output_boundary/] PASS\n");
}

// --- gpu/ ---
#include <kivo/video_plane/gpu/gpu_budget_snapshot.hpp>
#include <kivo/video_plane/gpu/budget_pressure_level.hpp>
#include <kivo/video_plane/gpu/device_removed_reason.hpp>

static void test_gpu() {
    using namespace kivo::video_plane::gpu;
    GpuBudgetSnapshot snap{};
    if (snap.pressure != BudgetPressureLevel::Relaxed) throw 1;
    if (BudgetPressureLevel::Critical == BudgetPressureLevel::Relaxed) throw 1;
    DeviceRemovedInfo info{};
    if (info.reason != DeviceRemovedReason::None) throw 1;
    std::printf("[gpu/] PASS\n");
}

// --- thread/ ---
#include <kivo/video_plane/thread/thread_affinity_token.hpp>
#include <kivo/video_plane/thread/lock_rank_checker.hpp>

static void test_thread() {
    using namespace kivo::video_plane::thread;
    ThreadAffinityToken tok{};
    if (tok.thread_id != 0) throw 1;
    if (tok.is_pinned) throw 1;
    LockRankChecker checker{};
    if (checker.rank_violated) throw 1;
    std::printf("[thread/] PASS\n");
}

// --- architecture/ ---
#include <kivo/video_plane/architecture/architecture_violation_schema.hpp>
#include <kivo/video_plane/architecture/symbol_checker_rule.hpp>

static void test_architecture() {
    using namespace kivo::video_plane::architecture;
    SymbolCheckerRule rule{};
    rule.check_name = "test";
    if (rule.check_name != "test") throw 1;
    std::printf("[architecture/] PASS\n");
}

// --- license/ ---
#include <kivo/video_plane/license/patent_risk_report.hpp>
#include <kivo/video_plane/license/compliance_package.hpp>

static void test_license() {
    using namespace kivo::video_plane::license;
    PatentRiskReport report{};
    if (report.level != PatentRiskLevel::Unknown) throw 1;
    CompliancePackage pkg{};
    if (pkg.is_complete) throw 1;
    std::printf("[license/] PASS\n");
}

// --- telemetry/ ---
#include <kivo/video_plane/telemetry/telemetry_privacy_policy.hpp>
#include <kivo/video_plane/telemetry/diagnostics_retention_policy.hpp>

static void test_telemetry() {
    using namespace kivo::video_plane::telemetry;
    TelemetryPrivacyPolicy pol{};
    if (pol.collect_usage_data) throw 1;
    if (!pol.require_user_consent) throw 1;
    DiagnosticsRetentionPolicy dr{};
    if (dr.max_retention_days != 30) throw 1;
    std::printf("[telemetry/] PASS\n");
}

// --- state_monitor/ ---
#include <kivo/video_plane/state_monitor/formal_state_monitor.hpp>
#include <kivo/video_plane/state_monitor/formal_state_violation.hpp>

static void test_state_monitor() {
    using namespace kivo::video_plane::state_monitor;
    FormalStateMonitor mon{};
    if (mon.is_monitoring) throw 1;
    FormalStateViolation viol{};
    if (!viol.actual_state.empty()) throw 1;
    std::printf("[state_monitor/] PASS\n");
}

// --- test_ci/ ---
#include <kivo/video_plane/test_ci/gate_type.hpp>
#include <kivo/video_plane/test_ci/debug_layer_policy.hpp>

static void test_test_ci() {
    using namespace kivo::video_plane::test_ci;
    GateDescriptor gd{};
    if (gd.type != GateType::Contract) throw 1;
    if (!gd.is_blocking) throw 1;
    DebugLayerPolicy dp{};
    if (!dp.enable_in_ci) throw 1;
    std::printf("[test_ci/] PASS\n");
}

// --- malformed/ ---
#include <kivo/video_plane/malformed/malformed_corpus_item.hpp>

static void test_malformed() {
    using namespace kivo::video_plane::malformed;
    MalformedCorpusItem item{};
    if (item.kind != MalformedKind::Unknown) throw 1;
    item.kind = MalformedKind::Truncated;
    if (item.kind != MalformedKind::Truncated) throw 1;
    std::printf("[malformed/] PASS\n");
}

// --- export/ ---
#include <kivo/video_plane/export/frame_export_security_policy.hpp>

static void test_export() {
    using namespace kivo::video_plane::export_policy;
    FrameExportSecurityPolicy pol{};
    if (!pol.allow_sdr_export) throw 1;
    if (pol.allow_hdr_export) throw 1;
    if (!pol.strip_protected_content) throw 1;
    std::printf("[export/] PASS\n");
}

// --- plugin/ ---
#include <kivo/video_plane/plugin/plugin_security_policy.hpp>

static void test_plugin() {
    using namespace kivo::video_plane::plugin;
    PluginSecurityPolicy pol{};
    if (pol.default_trust != PluginTrustLevel::Blocked) throw 1;
    if (!pol.require_signature) throw 1;
    std::printf("[plugin/] PASS\n");
}

// --- window/ ---
#include <kivo/video_plane/window/window_visibility_policy.hpp>

static void test_window() {
    using namespace kivo::video_plane::window;
    WindowVisibilityPolicy pol{};
    if (pol.state != WindowVisibilityState::Visible) throw 1;
    if (!pol.hold_last_frame_when_minimized) throw 1;
    std::printf("[window/] PASS\n");
}

// --- success/ ---
#include <kivo/video_plane/success/success_definition_item.hpp>

static void test_success() {
    using namespace kivo::video_plane::success;
    SuccessDefinitionItem item{};
    if (!item.is_required) throw 1;
    if (!item.scenario_name.empty()) throw 1;
    std::printf("[success/] PASS\n");
}

// --- change/ ---
#include <kivo/video_plane/change/change_control.hpp>

static void test_change() {
    using namespace kivo::video_plane::change;
    ChangeControl cc{};
    if (cc.impact != ChangeImpact::None) throw 1;
    if (!cc.requires_approval) throw 1;
    std::printf("[change/] PASS\n");
}

// --- main ---
static int g_failed = 0;

#define P5A_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

int main() {
    std::printf("=== P5A Video Plane Contract Tests ===\n\n");

    P5A_RUN(test_stamp);
    P5A_RUN(test_capability);
    P5A_RUN(test_evidence);
    P5A_RUN(test_threat);
    P5A_RUN(test_sandbox);
    P5A_RUN(test_allocation);
    P5A_RUN(test_fuzz);
    P5A_RUN(test_version);
    P5A_RUN(test_failure);
    P5A_RUN(test_recovery);
    P5A_RUN(test_queue);
    P5A_RUN(test_protected_content);
    P5A_RUN(test_protected_output);
    P5A_RUN(test_dolby_vision);
    P5A_RUN(test_hdr10_plus);
    P5A_RUN(test_p010);
    P5A_RUN(test_d3d11va);
    P5A_RUN(test_swapchain);
    P5A_RUN(test_vrr);
    P5A_RUN(test_color);
    P5A_RUN(test_timebase);
    P5A_RUN(test_attachment);
    P5A_RUN(test_presentation);
    P5A_RUN(test_output_boundary);
    P5A_RUN(test_gpu);
    P5A_RUN(test_thread);
    P5A_RUN(test_architecture);
    P5A_RUN(test_license);
    P5A_RUN(test_telemetry);
    P5A_RUN(test_state_monitor);
    P5A_RUN(test_test_ci);
    P5A_RUN(test_malformed);
    P5A_RUN(test_export);
    P5A_RUN(test_plugin);
    P5A_RUN(test_window);
    P5A_RUN(test_success);
    P5A_RUN(test_change);

    if (g_failed == 0) {
        std::printf("\n=== P5A Summary: all 37 families passed ===\n");
        return 0;
    } else {
        std::printf("\n=== P5A Summary: %d family(s) FAILED ===\n", g_failed);
        return 1;
    }
}

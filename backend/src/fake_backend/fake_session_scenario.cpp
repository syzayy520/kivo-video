#include "kivo/cinema_engine/fake_backend/fake_session_scenario.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace kivo::cinema_engine {

namespace {

constexpr const char* kTraceId = "trace.fake.direct-play.0001";
constexpr const char* kSessionId = "session.fake.0001";
constexpr const char* kCreatedAt = "2026-06-24T00:00:00Z";

ContractMetadata metadata() {
  return make_contract_metadata(kTraceId, kSessionId);
}

DecisionLedgerEntry ledger_entry(std::uint64_t sequence_number,
                                 DecisionLedgerEntryKind kind,
                                 std::string actor,
                                 std::string rationale) {
  DecisionLedgerEntry entry;
  entry.metadata = metadata();
  entry.ledger_id = "ledger.fake.0001";
  entry.entry_id = "ledger.fake.entry." + std::to_string(sequence_number);
  entry.session_id = kSessionId;
  entry.sequence_number = sequence_number;
  entry.generation = 1;
  entry.entry_kind = kind;
  entry.actor = std::move(actor);
  entry.rationale = std::move(rationale);
  entry.created_at = kCreatedAt;
  return entry;
}

}  // namespace

FakeSessionScenario build_fake_direct_play_session() {
  FakeSessionScenario scenario;

  scenario.remote_object_identity.metadata = metadata();
  scenario.remote_object_identity.remote_object_identity_id = "roi.fake.webdav.movie";
  scenario.remote_object_identity.source_id = "source.webdav.primary";
  scenario.remote_object_identity.source_kind = "WebDavPlayable";
  scenario.remote_object_identity.provider_kind = "WebDAV";
  scenario.remote_object_identity.canonical_path = "/Movies/Kivo.Sample.mkv";
  scenario.remote_object_identity.content_length = 81000000000ULL;
  scenario.remote_object_identity.etag = "etag-kivo-sample";
  scenario.remote_object_identity.stable_identity_confidence = 0.98;
  scenario.remote_object_identity.range_behavior = RangeBehavior::Verified;
  scenario.remote_object_identity.direct_link_lifetime = DirectLinkLifetime::TtlKnown;
  scenario.remote_object_identity.refresh_strategy = RefreshStrategy::RefreshSignedUrl;
  scenario.remote_object_identity.cache_identity_key = "cache.roi.fake.webdav.movie";

  scenario.source.metadata = metadata();
  scenario.source.source_id = "source.webdav.primary";
  scenario.source.source_kind = "WebDavPlayable";
  scenario.source.playable_object_id = "playable.webdav.kivo-sample";
  scenario.source.identity = "webdav://server/Movies/Kivo.Sample.mkv";
  scenario.source.access_policy = "credential-handle-only";
  scenario.source.credential_handle_ref = "secret.credential.webdav.primary";
  scenario.source.lifetime = "ttl-known";
  scenario.source.refresh_policy = "refresh-signed-url-before-expiry";
  scenario.source.capability_summary = "range-verified;direct-play-candidate";
  scenario.source.remote_object_identity_ref = scenario.remote_object_identity.remote_object_identity_id;

  scenario.cache_extent.cache_extent_id = "cache.extent.0001";
  scenario.cache_extent.remote_object_identity_ref = scenario.remote_object_identity.remote_object_identity_id;
  scenario.cache_extent.start_byte = 0;
  scenario.cache_extent.end_byte = 4194303;

  scenario.capability_join_result.metadata = metadata();
  scenario.capability_join_result.join_result_id = "cap.join.fake.0001";
  scenario.capability_join_result.session_id = kSessionId;
  scenario.capability_join_result.static_capability_ref = "cap.static.fake.0001";
  scenario.capability_join_result.device_capability_ref = "cap.device.fake.hdr-display";
  scenario.capability_join_result.session_capability_ref = "cap.session.fake.webdav";
  scenario.capability_join_result.runtime_observation_refs = {"cap.runtime.fake.throughput"};
  scenario.capability_join_result.confidence = 0.92;
  scenario.capability_join_result.expiry = "session";

  scenario.compliance_gate_result.metadata = metadata();
  scenario.compliance_gate_result.compliance_gate_id = "compliance.fake.0001";
  scenario.compliance_gate_result.compliance_trace_id = "compliance.trace.fake.dv-aware";
  scenario.compliance_gate_result.claim_level = "dv_metadata_detected";
  scenario.compliance_gate_result.allowed_claims = {"Dolby Vision-aware", "DV metadata detected"};
  scenario.compliance_gate_result.forbidden_claim_reasons = {"official Dolby Vision output requires certification"};
  scenario.compliance_gate_result.certification_state = "not-certified";

  scenario.feature_gate_snapshot.metadata = metadata();
  scenario.feature_gate_snapshot.feature_gate_snapshot_id = "fg.fake.0001";
  scenario.feature_gate_snapshot.enabled_features = {"direct_play", "range_cache", "playback_inspector"};
  scenario.feature_gate_snapshot.experimental_features = {"dv_aware_compatibility_mapping"};
  scenario.feature_gate_snapshot.compliance_limited_features = {"dolby_claims"};
  scenario.feature_gate_snapshot.reason = "P1 fake backend baseline";
  scenario.feature_gate_snapshot.created_at = kCreatedAt;

  scenario.resource_budget_result.metadata = metadata();
  scenario.resource_budget_result.resource_budget_id = "resource.fake.0001";
  scenario.resource_budget_result.cpu_budget = "normal";
  scenario.resource_budget_result.gpu_budget = "hdr-presenter-available";
  scenario.resource_budget_result.memory_budget = "sufficient";
  scenario.resource_budget_result.network_budget = "4k-remote-direct-play";
  scenario.resource_budget_result.playback_allowed = true;

  scenario.media_experience_snapshot.metadata = metadata();
  scenario.media_experience_snapshot.media_experience_snapshot_id = "experience.fake.0001";
  scenario.media_experience_snapshot.media_id = "media.fake.kivo-sample";
  scenario.media_experience_snapshot.version_id = "version.fake.4k-hdr";
  scenario.media_experience_snapshot.title = "Kivo Sample";
  scenario.media_experience_snapshot.year = 2026;
  scenario.media_experience_snapshot.playback_progress = 0.0;
  scenario.media_experience_snapshot.watched_state = "unwatched";
  scenario.media_experience_snapshot.version_summary = "4K HDR remote source";
  scenario.media_experience_snapshot.source_availability_summary = "primary WebDAV range verified";
  scenario.media_experience_snapshot.subtitle_availability_summary = "ASS and SRT available";
  scenario.media_experience_snapshot.audio_quality_summary = "TrueHD passthrough candidate";
  scenario.media_experience_snapshot.video_quality_summary = "HDR10 plus DV metadata detected";
  scenario.media_experience_snapshot.hdr_badge_claim_ref = scenario.compliance_gate_result.compliance_gate_id;
  scenario.media_experience_snapshot.user_visible_quality_badges = {"4K", "HDR10", "Dolby Vision-aware"};
  scenario.media_experience_snapshot.metadata_provenance = "fake-local-fixture";

  scenario.color_output_matrix.metadata = metadata();
  scenario.color_output_matrix.matrix_id = "color.matrix.fake.0001";
  scenario.color_output_matrix.session_id = kSessionId;
  scenario.color_output_matrix.input_signal = "DV_Profile8";
  scenario.color_output_matrix.preserved_metadata = "mastering_display,max_cll,max_fall,rpu_presence";
  scenario.color_output_matrix.target_display_class = "HDR10_PQ_BT2020";
  scenario.color_output_matrix.output_path = "DV_aware_experimental_mapping";
  scenario.color_output_matrix.output_transfer = "PQ";
  scenario.color_output_matrix.output_color_space = "BT2020";
  scenario.color_output_matrix.tone_mapping_mode = "metadata-aware";
  scenario.color_output_matrix.gamut_mapping_mode = "preserve-bt2020";
  scenario.color_output_matrix.hdr_metadata_policy = "set-hdr10-static-metadata";
  scenario.color_output_matrix.dv_fallback_tier = "dv-aware-compatibility";
  scenario.color_output_matrix.claim_level = "dv_metadata_detected";
  scenario.color_output_matrix.quality_tier = "High";
  scenario.color_output_matrix.compliance_gate_ref = scenario.compliance_gate_result.compliance_gate_id;

  scenario.windows_hdr_output_contract.metadata = metadata();
  scenario.windows_hdr_output_contract.windows_hdr_output_id = "winhdr.fake.0001";
  scenario.windows_hdr_output_contract.session_id = kSessionId;
  scenario.windows_hdr_output_contract.display_id = "display.fake.primary";
  scenario.windows_hdr_output_contract.monitor_luid = "monitor.fake.luid";
  scenario.windows_hdr_output_contract.gpu_adapter_id = "gpu.fake.adapter";
  scenario.windows_hdr_output_contract.advanced_color_available = true;
  scenario.windows_hdr_output_contract.hdr_enabled = true;
  scenario.windows_hdr_output_contract.color_space = "DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
  scenario.windows_hdr_output_contract.swapchain_format = "R10G10B10A2_UNORM";
  scenario.windows_hdr_output_contract.output_bit_depth = 10;
  scenario.windows_hdr_output_contract.fullscreen_state = "borderless";
  scenario.windows_hdr_output_contract.window_occlusion_state = "visible";
  scenario.windows_hdr_output_contract.multi_monitor_state = "single-active-hdr";
  scenario.windows_hdr_output_contract.os_tone_mapping_mode = "advanced-color";
  scenario.windows_hdr_output_contract.hdr_metadata_set_result = "fake-success";
  scenario.windows_hdr_output_contract.display_luminance_caps = "min-0.005,max-1000";
  scenario.windows_hdr_output_contract.target_display_class = "HDR10_PQ_BT2020";
  scenario.windows_hdr_output_contract.color_output_matrix_ref = scenario.color_output_matrix.matrix_id;
  scenario.windows_hdr_output_contract.output_confidence = 0.9;

  scenario.video_presenter_contract.metadata = metadata();
  scenario.video_presenter_contract.presenter_contract_id = "presenter.fake.0001";
  scenario.video_presenter_contract.session_id = kSessionId;
  scenario.video_presenter_contract.presenter_backend = "fake-d3d-adapter";
  scenario.video_presenter_contract.vsync_source = "display-vblank";
  scenario.video_presenter_contract.refresh_rate = 23.976;
  scenario.video_presenter_contract.content_frame_rate = 23.976;
  scenario.video_presenter_contract.frame_pacing_mode = "exact-24p";
  scenario.video_presenter_contract.clock_master = "audio_clock";
  scenario.video_presenter_contract.late_frame_policy = "drop-if-over-budget";
  scenario.video_presenter_contract.drop_duplicate_policy = "preserve-cadence";
  scenario.video_presenter_contract.jitter_budget = "2ms";
  scenario.video_presenter_contract.zero_copy_state = "available";
  scenario.video_presenter_contract.hdr_output_ref = scenario.windows_hdr_output_contract.windows_hdr_output_id;
  scenario.video_presenter_contract.color_output_matrix_ref = scenario.color_output_matrix.matrix_id;
  scenario.video_presenter_contract.expected_pacing_quality = "Reference";

  scenario.subtitle_render_plan.metadata = metadata();
  scenario.subtitle_render_plan.subtitle_render_plan_id = "subtitle.fake.0001";
  scenario.subtitle_render_plan.session_id = kSessionId;
  scenario.subtitle_render_plan.subtitle_stream_id = "sub.ass.jpn";
  scenario.subtitle_render_plan.subtitle_kind = "text_ass";
  scenario.subtitle_render_plan.renderer_backend = "libass-adapter";
  scenario.subtitle_render_plan.font_resolution_policy = "embedded-first";
  scenario.subtitle_render_plan.embedded_font_policy = "fallback-with-reason";
  scenario.subtitle_render_plan.ass_override_policy = "respect-script";
  scenario.subtitle_render_plan.hdr_luminance_policy = "tone-map-subtitle-luma";
  scenario.subtitle_render_plan.safe_area_policy = "content-safe-area";
  scenario.subtitle_render_plan.timing_correction = "none";
  scenario.subtitle_render_plan.user_offset = "0ms";
  scenario.subtitle_render_plan.language_policy = "user-preferred";
  scenario.subtitle_render_plan.forced_subtitle_policy = "auto-forced";
  scenario.subtitle_render_plan.default_subtitle_policy = "prefer-matched-audio";

  scenario.audio_output_decision.metadata = metadata();
  scenario.audio_output_decision.audio_output_decision_id = "audio.fake.0001";
  scenario.audio_output_decision.session_id = kSessionId;
  scenario.audio_output_decision.selected_audio_stream = "audio.truehd.7.1";
  scenario.audio_output_decision.codec = "TrueHD";
  scenario.audio_output_decision.channel_layout = "7.1";
  scenario.audio_output_decision.sample_rate = 48000;
  scenario.audio_output_decision.bit_depth = 24;
  scenario.audio_output_decision.passthrough_requested = true;
  scenario.audio_output_decision.passthrough_allowed = true;
  scenario.audio_output_decision.passthrough_result = "allowed";
  scenario.audio_output_decision.pcm_decode_result = "available-fallback";
  scenario.audio_output_decision.endpoint_capability_ref = "audio.endpoint.fake.hdmi";
  scenario.audio_output_decision.avr_capability_ref = "avr.fake.truehd";
  scenario.audio_output_decision.output_mode = "passthrough";
  scenario.audio_output_decision.sync_offset = "0ms";

  scenario.playback_plan.metadata = metadata();
  scenario.playback_plan.plan_id = "plan.fake.0001";
  scenario.playback_plan.plan_version = "1";
  scenario.playback_plan.session_id = kSessionId;
  scenario.playback_plan.intent = "open-and-play";
  scenario.playback_plan.selected_version = "version.fake.4k-hdr";
  scenario.playback_plan.selected_source = {
      scenario.source.source_id,
      scenario.source.source_kind,
      scenario.source.playable_object_id,
      scenario.remote_object_identity.remote_object_identity_id,
      0.94,
      0.91,
      "verified range remote direct play",
      "verified",
      "ttl-known",
      "sufficient-for-4k",
      "low",
      "low",
      "medium",
      "refresh-direct-link"};
  scenario.playback_plan.rejected_sources = {{"source.emby.proxy", "EmbyProxyPlayable", "lower quality than verified WebDAV direct play", "would proxy remote stream", "higher diagnostic replay cost", "Verified range direct source selected first.", "trace.reject.emby.proxy"}};
  scenario.playback_plan.remote_object_identity_ref = scenario.remote_object_identity.remote_object_identity_id;
  scenario.playback_plan.capability_join_result_ref = scenario.capability_join_result.join_result_id;
  scenario.playback_plan.policy_decision_ref = "policy.fake.direct-play-first";
  scenario.playback_plan.compliance_gate_ref = scenario.compliance_gate_result.compliance_gate_id;
  scenario.playback_plan.feature_gate_snapshot_ref = scenario.feature_gate_snapshot.feature_gate_snapshot_id;
  scenario.playback_plan.resource_budget_ref = scenario.resource_budget_result.resource_budget_id;
  scenario.playback_plan.media_experience_snapshot_ref = scenario.media_experience_snapshot.media_experience_snapshot_id;
  scenario.playback_plan.selected_tracks = {"video.hevc.main10", "audio.truehd.7.1", "sub.ass.jpn"};
  scenario.playback_plan.decode_plan = "hardware-hevc-main10-preferred";
  scenario.playback_plan.audio_plan = scenario.audio_output_decision.audio_output_decision_id;
  scenario.playback_plan.subtitle_plan = scenario.subtitle_render_plan.subtitle_render_plan_id;
  scenario.playback_plan.color_plan = scenario.color_output_matrix.matrix_id;
  scenario.playback_plan.windows_hdr_output_plan = scenario.windows_hdr_output_contract.windows_hdr_output_id;
  scenario.playback_plan.presentation_timing_plan = scenario.video_presenter_contract.presenter_contract_id;
  scenario.playback_plan.cache_plan = "range-cache-bound-to-remote-object-identity";
  scenario.playback_plan.graph_plan = "fake-direct-play-graph";
  scenario.playback_plan.fallback_chain = {"refresh-direct-link", "disable-range-cache", "software-decode", "reject-if-identity-conflict"};
  scenario.playback_plan.recovery_actions = {"DIRECT_LINK_REFRESH_ATTEMPTED", "RANGE_CACHE_DISABLED_REASON", "HW_DECODE_TO_SW_DECODE"};
  scenario.playback_plan.expected_quality_tier = "High";
  scenario.playback_plan.risk_tier = "Normal";
  scenario.playback_plan.user_visible_summary = "Direct Play with verified remote identity and HDR output.";
  scenario.playback_plan.technical_trace_ref = "trace.fake.direct-play.plan";

  scenario.playback_inspector_snapshot.metadata = metadata();
  scenario.playback_inspector_snapshot.inspector_snapshot_id = "inspector.fake.0001";
  scenario.playback_inspector_snapshot.session_id = kSessionId;
  scenario.playback_inspector_snapshot.user_summary = "Direct Play selected because range and identity are verified.";
  scenario.playback_inspector_snapshot.source_summary = "WebDAV source with stable remote object identity.";
  scenario.playback_inspector_snapshot.network_summary = "Throughput sufficient for remote 4K.";
  scenario.playback_inspector_snapshot.cache_summary = "Range cache bound to RemoteObjectIdentity.";
  scenario.playback_inspector_snapshot.video_summary = "HEVC Main10 fake decode path.";
  scenario.playback_inspector_snapshot.hdr_dv_summary = "DV metadata detected; DV-aware compatibility mapping.";
  scenario.playback_inspector_snapshot.audio_summary = "TrueHD passthrough selected.";
  scenario.playback_inspector_snapshot.subtitle_summary = "ASS render plan selected.";
  scenario.playback_inspector_snapshot.fallback_summary = "Recovery chain prepared, no silent fallback.";
  scenario.playback_inspector_snapshot.quality_impact_summary = "No quality downgrade in fake baseline.";
  scenario.playback_inspector_snapshot.actions_available = {"refresh-source", "export-diagnostic"};
  scenario.playback_inspector_snapshot.developer_trace_ref = "developer.trace.fake.0001";
  scenario.playback_inspector_snapshot.privacy_level = "redacted";
  scenario.playback_inspector_snapshot.redaction_state = "clean";

  scenario.diagnostic_bundle_manifest.metadata = metadata();
  scenario.diagnostic_bundle_manifest.diagnostic_bundle_id = "diagnostic.fake.0001";
  scenario.diagnostic_bundle_manifest.redaction_profile = "developer-safe";
  scenario.diagnostic_bundle_manifest.redaction_state = "clean";
  scenario.diagnostic_bundle_manifest.included_refs = {scenario.playback_plan.plan_id, scenario.playback_inspector_snapshot.inspector_snapshot_id};
  scenario.diagnostic_bundle_manifest.excluded_secret_kinds = {"token", "cookie", "signed_url", "credential"};

  scenario.session.metadata = metadata();
  scenario.session.session_id = kSessionId;
  scenario.session.playback_intent_id = "intent.fake.open";
  scenario.session.media_id = scenario.media_experience_snapshot.media_id;
  scenario.session.version_id = "version.fake.4k-hdr";
  scenario.session.selected_source_id = scenario.source.source_id;
  scenario.session.plan_id = scenario.playback_plan.plan_id;
  scenario.session.graph_id = "graph.fake.0001";
  scenario.session.capability_join_result_id = scenario.capability_join_result.join_result_id;
  scenario.session.policy_decision_id = scenario.playback_plan.policy_decision_ref;
  scenario.session.compliance_gate_id = scenario.compliance_gate_result.compliance_gate_id;
  scenario.session.feature_gate_snapshot_id = scenario.feature_gate_snapshot.feature_gate_snapshot_id;
  scenario.session.resource_budget_id = scenario.resource_budget_result.resource_budget_id;
  scenario.session.media_experience_snapshot_id = scenario.media_experience_snapshot.media_experience_snapshot_id;
  scenario.session.color_output_matrix_id = scenario.color_output_matrix.matrix_id;
  scenario.session.windows_hdr_output_contract_id = scenario.windows_hdr_output_contract.windows_hdr_output_id;
  scenario.session.video_presenter_contract_id = scenario.video_presenter_contract.presenter_contract_id;
  scenario.session.subtitle_render_plan_id = scenario.subtitle_render_plan.subtitle_render_plan_id;
  scenario.session.audio_output_decision_id = scenario.audio_output_decision.audio_output_decision_id;
  scenario.session.playback_inspector_snapshot_id = scenario.playback_inspector_snapshot.inspector_snapshot_id;
  scenario.session.trace_root_id = kTraceId;
  scenario.session.decision_ledger_id = "ledger.fake.0001";
  scenario.session.created_at = kCreatedAt;
  scenario.session.closed_at = "2026-06-24T00:00:10Z";
  scenario.session.close_reason = "fake-session-complete";

  scenario.decision_ledger = {
      ledger_entry(1, DecisionLedgerEntryKind::CommandAccepted, "EngineApi", "Open command accepted."),
      ledger_entry(2, DecisionLedgerEntryKind::RemoteObjectIdentified, "RemoteObjectCore", "Remote object identity verified before cache use."),
      ledger_entry(3, DecisionLedgerEntryKind::CapabilityJoined, "CapabilityCore", "Layered capabilities joined for planning."),
      ledger_entry(4, DecisionLedgerEntryKind::ComplianceChecked, "ComplianceCore", "Dolby claims limited to DV-aware compatibility."),
      ledger_entry(5, DecisionLedgerEntryKind::FeatureGateSnapshotted, "FeatureGateCore", "Feature gates frozen for replay."),
      ledger_entry(6, DecisionLedgerEntryKind::MediaExperienceSnapshotted, "MediaExperienceCore", "Quality badges sourced from experience and compliance."),
      ledger_entry(7, DecisionLedgerEntryKind::ColorOutputMatrixCreated, "ColorScienceCore", "HDR output matrix selected."),
      ledger_entry(8, DecisionLedgerEntryKind::WindowsHdrOutputEvaluated, "WindowsHdrCore", "Windows HDR output contract produced."),
      ledger_entry(9, DecisionLedgerEntryKind::PresenterPlanSelected, "VideoRenderCore", "23.976 pacing selected."),
      ledger_entry(10, DecisionLedgerEntryKind::SubtitleRenderPlanSelected, "SubtitleCore", "ASS subtitle render plan selected."),
      ledger_entry(11, DecisionLedgerEntryKind::AudioOutputDecisionSelected, "AudioCore", "TrueHD passthrough selected."),
      ledger_entry(12, DecisionLedgerEntryKind::PlaybackPlanCreated, "PlaybackPlanCore", "Direct Play plan created with fallback chain."),
      ledger_entry(13, DecisionLedgerEntryKind::InspectorSnapshotCreated, "PlaybackInspectorCore", "Inspector snapshot created."),
      ledger_entry(14, DecisionLedgerEntryKind::SessionClosed, "SessionCore", "Fake session closed with replayable ledger.")};

  return scenario;
}

bool fake_session_is_replayable(const FakeSessionScenario& scenario) {
  if (scenario.decision_ledger.empty()) {
    return false;
  }

  std::uint64_t expected_sequence = 1;
  for (const auto& entry : scenario.decision_ledger) {
    if (entry.sequence_number != expected_sequence || !entry.generation.has_value()) {
      return false;
    }
    ++expected_sequence;
  }

  return scenario.session.plan_id.has_value() &&
         scenario.session.decision_ledger_id == "ledger.fake.0001" &&
         scenario.playback_plan.feature_gate_snapshot_ref == scenario.feature_gate_snapshot.feature_gate_snapshot_id &&
         scenario.cache_extent.remote_object_identity_ref == scenario.remote_object_identity.remote_object_identity_id &&
         scenario.color_output_matrix.compliance_gate_ref == scenario.compliance_gate_result.compliance_gate_id &&
         scenario.media_experience_snapshot.hdr_badge_claim_ref == scenario.compliance_gate_result.compliance_gate_id;
}

}  // namespace kivo::cinema_engine

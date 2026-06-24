#include "kivo/cinema_engine/playback_inspector_core/fake_inspector_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

PlaybackInspectorSnapshot build_fake_playback_inspector_snapshot(
    const ContractMetadata& metadata,
    const ComplianceGateResult& compliance_gate,
    std::string user_summary,
    std::string source_summary,
    std::string network_summary) {
  PlaybackInspectorSnapshot snapshot;
  snapshot.metadata = metadata;
  snapshot.inspector_snapshot_id = "inspector.fake.service.0001";
  snapshot.session_id = metadata.session_id;
  snapshot.user_summary = std::move(user_summary);
  snapshot.source_summary = std::move(source_summary);
  snapshot.network_summary = std::move(network_summary);
  snapshot.cache_summary = "cache.fake.hit_rate=95%";
  snapshot.video_summary = "video.fake.4k_hdr_dv";
  snapshot.hdr_dv_summary = "hdr.fake.dv_profile5";
  snapshot.audio_summary = "audio.fake.truehd_7.1";
  snapshot.subtitle_summary = "subtitle.fake.ass_external";
  snapshot.fallback_summary = "fallback.fake.none";
  snapshot.quality_impact_summary = "quality.fake.optimal";
  snapshot.actions_available = {"pause", "seek", "switch_audio", "switch_subtitle", "toggle_hdr"};
  snapshot.developer_trace_ref = "trace.fake.inspector.0001";
  snapshot.privacy_level = "user_visible";
  snapshot.redaction_state = compliance_gate.fail_closed ? "redacted" : "unredacted";
  return snapshot;
}

}  // namespace kivo::cinema_engine

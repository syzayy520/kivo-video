#include "kivo/cinema_engine/audio_core/fake_audio_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

AudioOutputDecision build_fake_audio_output_decision(const ContractMetadata& metadata,
                                                     const ComplianceGateResult& compliance_gate,
                                                     std::string selected_audio_stream,
                                                     bool passthrough_requested) {
  AudioOutputDecision decision;
  decision.metadata = metadata;
  decision.audio_output_decision_id = "audio.fake.service.0001";
  decision.session_id = metadata.session_id;
  decision.selected_audio_stream = std::move(selected_audio_stream);
  decision.codec = "TrueHD";
  decision.channel_layout = "7.1";
  decision.sample_rate = 48000;
  decision.bit_depth = 24;
  decision.passthrough_requested = passthrough_requested;
  decision.passthrough_allowed = passthrough_requested && !compliance_gate.fail_closed;
  decision.passthrough_result = decision.passthrough_allowed ? "allowed" : "blocked";
  decision.pcm_decode_result = "available-fallback";
  decision.endpoint_capability_ref = "audio.endpoint.fake.hdmi";
  decision.avr_capability_ref = "avr.fake.truehd";
  decision.output_mode = decision.passthrough_allowed ? "passthrough" : "pcm_decode";
  decision.sync_offset = "0ms";
  if (!decision.passthrough_allowed) {
    decision.fallback_reason = "passthrough blocked by compliance or request state";
    decision.user_visible_hint = "Audio passthrough unavailable; PCM fallback selected.";
  }
  return decision;
}

}  // namespace kivo::cinema_engine


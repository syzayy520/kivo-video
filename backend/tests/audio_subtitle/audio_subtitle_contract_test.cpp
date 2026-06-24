#include <cassert>

#include "kivo/cinema_engine/audio_core/fake_audio_service.hpp"
#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/subtitle_core/fake_subtitle_service.hpp"

int main() {
  const auto metadata = kivo::cinema_engine::make_contract_metadata("trace.fake.audio-subtitle", "session.fake.0001");

  kivo::cinema_engine::ComplianceGateResult compliance_gate;
  compliance_gate.metadata = metadata;
  compliance_gate.compliance_gate_id = "compliance.fake.audio";
  compliance_gate.compliance_trace_id = "compliance.trace.fake.audio";
  compliance_gate.claim_level = "generic_playback";

  const auto subtitle_plan = kivo::cinema_engine::build_fake_subtitle_render_plan(
      metadata,
      "sub.ass.jpn",
      "text_ass");
  assert(subtitle_plan.metadata.schema_version == "1.1");
  assert(subtitle_plan.subtitle_kind == "text_ass");
  assert(subtitle_plan.embedded_font_policy == "fallback-with-reason");
  assert(subtitle_plan.hdr_luminance_policy == "tone-map-subtitle-luma");

  const auto audio_decision = kivo::cinema_engine::build_fake_audio_output_decision(
      metadata,
      compliance_gate,
      "audio.truehd.7.1",
      true);
  assert(audio_decision.metadata.schema_version == "1.1");
  assert(audio_decision.codec == "TrueHD");
  assert(audio_decision.passthrough_requested);
  assert(audio_decision.passthrough_allowed);
  assert(audio_decision.output_mode == "passthrough");

  compliance_gate.fail_closed = true;
  const auto fallback_audio = kivo::cinema_engine::build_fake_audio_output_decision(
      metadata,
      compliance_gate,
      "audio.truehd.7.1",
      true);
  assert(!fallback_audio.passthrough_allowed);
  assert(fallback_audio.output_mode == "pcm_decode");
  assert(fallback_audio.fallback_reason.has_value());
  assert(fallback_audio.user_visible_hint.has_value());

  return 0;
}


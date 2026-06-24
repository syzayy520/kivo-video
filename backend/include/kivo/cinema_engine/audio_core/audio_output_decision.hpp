#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct AudioOutputDecision {
  ContractMetadata metadata;
  std::string audio_output_decision_id;
  std::optional<std::string> session_id;
  std::string selected_audio_stream;
  std::string codec;
  std::string channel_layout;
  int sample_rate{0};
  std::optional<int> bit_depth;
  bool passthrough_requested{false};
  bool passthrough_allowed{false};
  std::string passthrough_result;
  std::string pcm_decode_result;
  std::string endpoint_capability_ref;
  std::optional<std::string> avr_capability_ref;
  std::string output_mode;
  std::string sync_offset;
  std::optional<std::string> lipsync_policy;
  std::optional<std::string> fallback_reason;
  std::optional<std::string> user_visible_hint;
};

}  // namespace kivo::cinema_engine


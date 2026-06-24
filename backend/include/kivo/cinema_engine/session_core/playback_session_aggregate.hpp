#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct PlaybackSessionAggregate {
  ContractMetadata metadata;
  std::string session_id;
  std::string playback_intent_id;
  std::string media_id;
  std::string version_id;
  std::optional<std::string> selected_source_id;
  std::optional<std::string> plan_id;
  std::optional<std::string> graph_id;
  std::optional<std::string> capability_join_result_id;
  std::optional<std::string> policy_decision_id;
  std::optional<std::string> compliance_gate_id;
  std::optional<std::string> feature_gate_snapshot_id;
  std::optional<std::string> resource_budget_id;
  std::optional<std::string> media_experience_snapshot_id;
  std::optional<std::string> color_output_matrix_id;
  std::optional<std::string> windows_hdr_output_contract_id;
  std::optional<std::string> video_presenter_contract_id;
  std::optional<std::string> subtitle_render_plan_id;
  std::optional<std::string> audio_output_decision_id;
  std::optional<std::string> playback_inspector_snapshot_id;
  std::string trace_root_id;
  std::string decision_ledger_id;
  std::string created_at;
  std::optional<std::string> closed_at;
  std::optional<std::string> close_reason;
};

}  // namespace kivo::cinema_engine


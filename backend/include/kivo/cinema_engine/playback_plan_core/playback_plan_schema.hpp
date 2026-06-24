#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SelectedSourcePlan {
  std::string source_id;
  std::string source_kind;
  std::string playable_object_ref;
  std::string remote_object_identity_ref;
  double rank_score{0.0};
  double rank_confidence{0.0};
  std::string dominant_reason;
  std::string range_status;
  std::string ttl_status;
  std::string expected_throughput;
  std::string startup_risk;
  std::string seek_risk;
  std::string rebuffer_risk;
  std::string recovery_cost;
};

struct RejectedSourcePlan {
  std::string source_id;
  std::string source_kind;
  std::string reject_reason;
  std::string quality_impact_if_selected;
  std::string risk_if_selected;
  std::string user_visible_hint;
  std::string technical_trace_ref;
};

struct PlaybackPlanSchema {
  ContractMetadata metadata;
  std::string plan_id;
  std::string plan_version;
  std::string session_id;
  std::string intent;
  std::string selected_version;
  SelectedSourcePlan selected_source;
  std::vector<RejectedSourcePlan> rejected_sources;
  std::string remote_object_identity_ref;
  std::string capability_join_result_ref;
  std::string policy_decision_ref;
  std::string compliance_gate_ref;
  std::string feature_gate_snapshot_ref;
  std::string resource_budget_ref;
  std::string media_experience_snapshot_ref;
  std::vector<std::string> selected_tracks;
  std::string decode_plan;
  std::string audio_plan;
  std::string subtitle_plan;
  std::string color_plan;
  std::string windows_hdr_output_plan;
  std::string presentation_timing_plan;
  std::string cache_plan;
  std::string graph_plan;
  std::vector<std::string> fallback_chain;
  std::vector<std::string> recovery_actions;
  std::string expected_quality_tier;
  std::string risk_tier;
  std::string user_visible_summary;
  std::string technical_trace_ref;
};

}  // namespace kivo::cinema_engine


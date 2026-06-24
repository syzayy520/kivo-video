#pragma once

#include <vector>

#include "kivo/cinema_engine/audio_core/audio_output_decision.hpp"
#include "kivo/cinema_engine/capability_core/capability_join_result.hpp"
#include "kivo/cinema_engine/color_science_core/color_output_matrix.hpp"
#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/decision_ledger_core/decision_ledger_entry.hpp"
#include "kivo/cinema_engine/diagnostic_core/diagnostic_bundle_manifest.hpp"
#include "kivo/cinema_engine/feature_gate_core/feature_gate_snapshot.hpp"
#include "kivo/cinema_engine/media_experience_core/media_experience_snapshot.hpp"
#include "kivo/cinema_engine/network_cache_core/cache_extent.hpp"
#include "kivo/cinema_engine/playback_inspector_core/playback_inspector_snapshot.hpp"
#include "kivo/cinema_engine/playback_plan_core/playback_plan_schema.hpp"
#include "kivo/cinema_engine/resource_core/resource_budget_result.hpp"
#include "kivo/cinema_engine/remote_object_core/remote_object_identity.hpp"
#include "kivo/cinema_engine/session_core/playback_session_aggregate.hpp"
#include "kivo/cinema_engine/source_core/source_playable_object.hpp"
#include "kivo/cinema_engine/subtitle_core/subtitle_render_plan.hpp"
#include "kivo/cinema_engine/video_render_core/video_presenter_contract.hpp"
#include "kivo/cinema_engine/windows_hdr_core/windows_hdr_output_contract.hpp"

namespace kivo::cinema_engine {

struct FakeSessionScenario {
  PlaybackSessionAggregate session;
  SourcePlayableObject source;
  RemoteObjectIdentity remote_object_identity;
  CacheExtent cache_extent;
  CapabilityJoinResult capability_join_result;
  ComplianceGateResult compliance_gate_result;
  FeatureGateSnapshot feature_gate_snapshot;
  ResourceBudgetResult resource_budget_result;
  MediaExperienceSnapshot media_experience_snapshot;
  PlaybackPlanSchema playback_plan;
  ColorOutputMatrix color_output_matrix;
  WindowsHdrOutputContract windows_hdr_output_contract;
  VideoPresenterContract video_presenter_contract;
  SubtitleRenderPlan subtitle_render_plan;
  AudioOutputDecision audio_output_decision;
  PlaybackInspectorSnapshot playback_inspector_snapshot;
  DiagnosticBundleManifest diagnostic_bundle_manifest;
  std::vector<DecisionLedgerEntry> decision_ledger;
};

FakeSessionScenario build_fake_direct_play_session();
bool fake_session_is_replayable(const FakeSessionScenario& scenario);

}  // namespace kivo::cinema_engine

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/foundation/reference.hpp"

namespace kivo::cinema_engine {

enum class DecisionLedgerEntryKind {
  CommandAccepted,
  PlaybackPlanCreated,
  SourceSelected,
  SourceRejected,
  RemoteObjectIdentified,
  CapabilityJoined,
  PolicyDecided,
  ComplianceChecked,
  FeatureGateSnapshotted,
  ResourceBudgetChecked,
  MediaExperienceSnapshotted,
  ColorOutputMatrixCreated,
  WindowsHdrOutputEvaluated,
  PresenterPlanSelected,
  SubtitleRenderPlanSelected,
  AudioOutputDecisionSelected,
  GraphBound,
  TrackSelected,
  FallbackTriggered,
  RecoveryActionSelected,
  PlanReevaluated,
  InspectorSnapshotCreated,
  SessionClosed
};

struct DecisionLedgerEntry {
  ContractMetadata metadata;
  std::string ledger_id;
  std::string entry_id;
  std::string session_id;
  std::uint64_t sequence_number{0};
  std::optional<std::uint64_t> generation;
  DecisionLedgerEntryKind entry_kind{DecisionLedgerEntryKind::CommandAccepted};
  std::string actor;
  std::optional<std::string> causation_id;
  std::optional<std::string> correlation_id;
  std::vector<ContractRef> input_refs;
  std::vector<ContractRef> output_refs;
  std::string rationale;
  std::optional<std::string> rollback_state;
  std::string created_at;
};

}  // namespace kivo::cinema_engine


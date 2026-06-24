#include "kivo/cinema_engine/foundation/contract_registry.hpp"

namespace kivo::cinema_engine {

const std::vector<ContractRegistration>& core_contract_registry() {
  static const std::vector<ContractRegistration> registry = {
      {"EngineCommand", "EngineApi", "1.1", ContractPersistence::AppendOnly, {"EngineApi"}, {"SessionCore", "PlaybackCore"}},
      {"EngineEvent", "EngineApi", "1.1", ContractPersistence::AppendOnly, {"EngineApi", "PlaybackCore"}, {"UI", "DiagnosticCore"}},
      {"PlaybackSessionSnapshot", "SessionCore", "1.1", ContractPersistence::Snapshot, {"SessionCore"}, {"EngineApi", "DiagnosticCore"}},
      {"PlaybackSessionAggregate", "SessionCore", "1.1", ContractPersistence::Persistent, {"SessionCore"}, {"PlaybackCore", "DecisionLedgerCore", "DiagnosticCore"}},
      {"DecisionLedgerEntry", "DecisionLedgerCore", "1.1", ContractPersistence::AppendOnly, {"DecisionLedgerCore"}, {"DiagnosticCore", "Replay"}},
      {"ReplayManifest", "DecisionLedgerCore", "1.1", ContractPersistence::ExportOnly, {"DecisionLedgerCore"}, {"DiagnosticCore", "Replay"}},
      {"PlaybackPlanSchema", "PlaybackPlanCore", "1.1", ContractPersistence::Persistent, {"PlaybackPlanCore"}, {"PlaybackCore", "GraphCore", "DiagnosticCore"}},
      {"CapabilityJoinResult", "CapabilityCore", "1.1", ContractPersistence::Persistent, {"CapabilityCore"}, {"PlaybackPlanCore", "DiagnosticCore"}},
      {"PolicyDecision", "PolicyCore", "1.1", ContractPersistence::Persistent, {"PolicyCore"}, {"PlaybackPlanCore", "DiagnosticCore"}},
      {"ComplianceGateResult", "ComplianceCore", "1.1", ContractPersistence::Persistent, {"ComplianceCore"}, {"PlaybackPlanCore", "MediaExperienceCore", "ColorScienceCore"}},
      {"FeatureGateSnapshot", "FeatureGateCore", "1.1", ContractPersistence::Persistent, {"FeatureGateCore"}, {"PlaybackPlanCore", "Replay"}},
      {"ResourceBudgetResult", "ResourceCore", "1.1", ContractPersistence::Persistent, {"ResourceCore"}, {"PlaybackPlanCore", "PlaybackCore"}},
      {"SourcePlayableObject", "SourceCore", "1.1", ContractPersistence::Persistent, {"SourceCore"}, {"PlaybackPlanCore", "DiagnosticCore"}},
      {"RemoteObjectIdentity", "RemoteObjectCore", "1.1", ContractPersistence::Persistent, {"RemoteObjectCore"}, {"SourceCore", "NetworkCacheCore", "PlaybackPlanCore"}},
      {"StreamInventory", "DemuxCore", "1.1", ContractPersistence::Persistent, {"DemuxCore"}, {"PlaybackPlanCore", "MediaExperienceCore"}},
      {"MediaExperienceSnapshot", "MediaExperienceCore", "1.1", ContractPersistence::Persistent, {"MediaExperienceCore"}, {"UI", "PlaybackPlanCore"}},
      {"ColorOutputMatrix", "ColorScienceCore", "1.1", ContractPersistence::Persistent, {"ColorScienceCore"}, {"WindowsHdrCore", "VideoRenderCore", "UI"}},
      {"WindowsHdrOutputContract", "WindowsHdrCore", "1.1", ContractPersistence::Persistent, {"WindowsHdrCore"}, {"VideoRenderCore", "UI"}},
      {"VideoPresenterContract", "VideoRenderCore", "1.1", ContractPersistence::Persistent, {"VideoRenderCore"}, {"PlaybackCore", "DiagnosticCore"}},
      {"SubtitleRenderPlan", "SubtitleCore", "1.1", ContractPersistence::Persistent, {"SubtitleCore"}, {"PlaybackPlanCore", "GraphCore", "UI"}},
      {"AudioOutputDecision", "AudioCore", "1.1", ContractPersistence::Persistent, {"AudioCore"}, {"PlaybackPlanCore", "GraphCore", "UI"}},
      {"PlaybackInspectorSnapshot", "PlaybackInspectorCore", "1.1", ContractPersistence::Snapshot, {"PlaybackInspectorCore"}, {"UI", "DiagnosticCore"}},
      {"DiagnosticBundleManifest", "DiagnosticCore", "1.1", ContractPersistence::ExportOnly, {"DiagnosticCore"}, {"DeveloperTools"}},
      {"EngineStateSnapshot", "EngineCore", "1.1", ContractPersistence::Snapshot, {"EngineCore"}, {"EngineApi", "UI", "DiagnosticCore"}},
      {"EngineError", "EngineCore", "1.1", ContractPersistence::AppendOnly, {"EngineCore", "PlaybackCore"}, {"UI", "DiagnosticCore"}},
  };
  return registry;
}

bool registry_contains_contract(const std::string& name) {
  for (const auto& contract : core_contract_registry()) {
    if (contract.name == name) {
      return true;
    }
  }
  return false;
}

}  // namespace kivo::cinema_engine

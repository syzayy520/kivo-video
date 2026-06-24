# Contract Registry

Every cross-module or persistent contract is registered before it can be used by a P1 fake backend path.

## Required Fields

- contract name
- owner module
- `schema_version`
- persistence class
- allowed writers
- allowed readers

## V1.1 Required Contracts

- EngineCommand
- EngineEvent
- PlaybackSessionSnapshot
- PlaybackSessionAggregate
- DecisionLedgerEntry
- PlaybackPlanSchema
- CapabilityJoinResult
- PolicyDecision
- ComplianceGateResult
- FeatureGateSnapshot
- ResourceBudgetResult
- SourcePlayableObject
- RemoteObjectIdentity
- StreamInventory
- MediaExperienceSnapshot
- ColorOutputMatrix
- WindowsHdrOutputContract
- VideoPresenterContract
- SubtitleRenderPlan
- AudioOutputDecision
- PlaybackInspectorSnapshot
- DiagnosticBundleManifest
- EngineStateSnapshot
- EngineError

## Gate

No P1 fake session may pass unless all required contracts exist in `ContractRegistry`.


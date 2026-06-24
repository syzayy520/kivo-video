# P1 Fake Backend Plan

P1 does not require real playback. It requires a fake backend that drives the full foundation.

## Milestones

1. Register every required cross-module contract.
2. Create and close a fake PlaybackSessionAggregate.
3. Write, query, and export DecisionLedger entries with sequence numbers and generations.
4. Instantiate PlaybackPlanSchema with selected source, rejected source, fallback chain, and recovery action.
5. Generate SourcePlayableObject and RemoteObjectIdentity for fake local, WebDAV, AList, and Emby sources.
6. Generate CapabilityJoinResult from layered capability refs.
7. Generate PolicyDecision, ComplianceGateResult, and FeatureGateSnapshot.
8. Generate MediaExperienceSnapshot and enforce badge origin.
9. Generate ColorOutputMatrix and claim-safe HDR/DV-aware output.
10. Generate WindowsHdrOutputContract and VideoPresenterContract.
11. Generate SubtitleRenderPlan and AudioOutputDecision.
12. Generate PlaybackInspectorSnapshot and DiagnosticBundleManifest.
13. Run governance gates and invariant shell tests.


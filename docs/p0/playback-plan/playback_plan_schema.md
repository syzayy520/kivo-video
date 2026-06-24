# Playback Plan Schema

PlaybackPlan is the frozen decision product that GraphCore executes.

## Inputs

PlaybackPlan reads only joined or decision contracts:

- `RemoteObjectIdentity`
- `CapabilityJoinResult`
- `PolicyDecision`
- `ComplianceGateResult`
- `FeatureGateSnapshot`
- `ResourceBudgetResult`
- `MediaExperienceSnapshot`

PlaybackPlan must not read raw static, device, session, or runtime capability snapshots.

## Required Outputs

- selected source
- rejected sources
- selected tracks
- decode plan
- audio plan
- subtitle plan
- color plan
- optional Windows HDR output plan
- presentation timing plan
- cache plan
- graph plan
- fallback chain
- recovery actions
- expected quality tier
- risk tier
- user-visible summary
- technical trace ref


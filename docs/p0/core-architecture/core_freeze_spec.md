# Core Freeze Spec

KivoCinemaEngine V1.1 freezes the backend foundation, not every long-term feature.

## Product Position

KivoCinemaEngine is the Windows private-cinema playback core. It chooses the highest reachable experience under the current source, network, device, display, audio endpoint, subtitle load, resource budget, compliance boundary, and user experience target. It must also prove why that choice is currently the highest reachable experience.

## Frozen V1.1 Surface

- First-level core list.
- Five authoritative states.
- Versioned core contracts.
- Remote object identity.
- Playback plan.
- Capability layering.
- DecisionLedger layering.
- ColorOutputMatrix.
- WindowsHdrOutputContract.
- VideoPresenterContract and frame pacing contract.
- SubtitleRenderPlan.
- AudioOutputDecision.
- MediaExperienceSnapshot.
- PlaybackInspectorSnapshot.
- GraphCore and PlaybackCore hard boundary.
- Fallback and recovery matrix.
- Minimal CI gates.
- P0 and P1 landing route.

## Non-Frozen V1.x Surface

Video quality, display calibration, audio theater, benchmark lab, compatibility farm, release engineering, plugin sandbox, data portability, accessibility, i18n, power and thermal adaptation, codec legal, cloud sync, and mobile companion stay in the V1.x expansion roadmap unless a minimal contract is explicitly required by V1.1.


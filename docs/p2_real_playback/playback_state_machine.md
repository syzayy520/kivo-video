# P2-005 Playback Session State Machine

Updated: 2026-06-25

## Goal

Formalize playback session state transitions with a real state machine contract that validates all transition rules, provides state metadata, and enforces invariants.

## State Transition Table

| From State | Trigger | To State | Condition |
|------------|---------|----------|-----------|
| Idle | open | Opening | session_id non-empty |
| Opening | session_created | CreatingSession | — |
| CreatingSession | source_resolved | ResolvingSource | — |
| ResolvingSource | version_selected | SelectingVersion | — |
| SelectingVersion | probe_planned | PlanningProbe | — |
| PlanningProbe | capability_joined | JoiningCapability | — |
| JoiningCapability | playback_planned | PlanningPlayback | — |
| PlanningPlayback | compliance_checked | CheckingCompliance | — |
| CheckingCompliance | feature_gate_checked | CheckingFeatureGate | — |
| CheckingFeatureGate | resource_budget_checked | CheckingResourceBudget | — |
| CheckingResourceBudget | graph_prepared | PreparingGraph | — |
| PreparingGraph | cache_prepared | PreparingCache | — |
| PreparingCache | media_probed | ProbingMedia | — |
| ProbingMedia | decoder_selected | SelectingDecoder | — |
| SelectingDecoder | render_prepared | PreparingRender | — |
| PreparingRender | ready | Ready | graph_bound |
| Ready | play | Playing | graph_bound |
| Playing | pause | Paused | — |
| Paused | play | Playing | — |
| Playing | seek | Seeking | seek_generation > 0 |
| Paused | seek | Seeking | seek_generation > 0 |
| Seeking | seek_complete | Playing/Paused | returns to pre-seek state |
| Playing | buffer | Buffering | — |
| Buffering | buffer_ready | Playing | — |
| Playing | source_switch | SwitchingSource | source_generation > 0 |
| SwitchingSource | switch_complete | Playing | — |
| Playing | track_switch | SwitchingTrack | track_generation > 0 |
| SwitchingTrack | switch_complete | Playing | — |
| Any | reevaluate | ReevaluatingPlan | — |
| ReevaluatingPlan | plan_accepted | Playing | — |
| Any | recover | Recovering | recovery_action_ref non-empty |
| Recovering | recovery_complete | Playing/Ready | depends on graph state |
| Playing | drain | Draining | — |
| Draining | drained | Ended | — |
| Any | fail | Failed | — |
| Failed | close | Closed | — |
| Any | close | Closed | — |

## Invariant Rules

1. **Session Required**: `Playing`, `Paused`, `Seeking`, `Buffering` require active session
2. **Graph Required**: `Playing` requires graph binding
3. **Seek Generation**: `Seeking` state requires `seek_generation > 0`
4. **Closed Terminal**: No transitions out of `Closed`
5. **Failed Recovery**: `Failed` can only go to `Closed`
6. **No Double Close**: Closing from `Closed` is invariant violation

## Implementation Plan

1. Create `PlaybackSessionStateMachine` contract with transition validation
2. Create state transition decision struct
3. Tests covering all valid transitions and invalid transition rejection

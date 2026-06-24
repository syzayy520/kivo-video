# Playback Session Model

SessionCore owns `PlaybackSessionState` and writes session identity.

## Authoritative Owner

Only SessionCore may write:

- `session_id`
- `media_id`
- `version_id`
- `plan_id`
- `graph_id`
- `trace_root_id`
- `decision_ledger_id`
- `created_at`
- `closed_at`
- `close_reason`

## Aggregate

`PlaybackSessionAggregate` joins the replay-critical references created by planning, capability, policy, compliance, feature gates, resource budget, media experience, color, HDR, presenter, subtitle, audio, and inspector modules.

## Rule

Session creation and the DecisionLedger root are a transaction. If session creation fails, PlaybackCore must not enter `Opening`.


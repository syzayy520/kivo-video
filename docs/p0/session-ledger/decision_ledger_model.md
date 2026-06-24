# Decision Ledger Model

DecisionLedgerCore records replay-critical causality. It is not a high-frequency telemetry sink.

## Entry Requirements

Each `DecisionLedgerEntry` includes:

- `schema_version`
- `producer_version`
- optional `replay_version`
- `ledger_id`
- `entry_id`
- `session_id`
- `sequence_number`
- optional `generation`
- `entry_kind`
- `actor`
- input refs
- output refs
- rationale
- optional rollback state
- `trace_id`
- `created_at`

## Not Allowed

Per-frame samples, queue watermarks, audio drift samples, throughput samples, and render jitter samples go to TelemetryTimeline or RingBuffer, not DecisionLedger.


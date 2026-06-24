# P1 Replay Manifest

ReplayManifest makes fake session replay evidence explicit.

## Scope

The manifest records:

- replay manifest id
- session id
- decision ledger id
- ordered replay-critical entry ids
- replayable flag
- replay blocker when the ledger is incomplete

## P1 Gate

The fake Engine API scenario must produce a replayable manifest from the same entries that were written to `DecisionLedgerStore`.


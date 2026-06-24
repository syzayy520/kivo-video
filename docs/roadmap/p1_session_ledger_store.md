# P1 Session And Ledger Store

P1 stores are in-memory and intentionally small. They prove contract behavior before SQLite domains are introduced.

## SessionStore

`SessionStore` supports:

- create PlaybackSessionAggregate
- close session
- find session by id
- export all sessions

## DecisionLedgerStore

`DecisionLedgerStore` supports:

- append replay-critical entry
- reject missing generation or invalid sequence
- reject duplicate entry id
- query entries by session id
- find entry by id
- export all entries

## Next Persistence Step

After P1 fake backend passes with a compiler, these stores can be backed by `playback.db` and `decision.db` without changing the contract types.


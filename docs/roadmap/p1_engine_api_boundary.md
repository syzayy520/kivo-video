# P1 Engine API Boundary

P1 starts the UI boundary as code. Callers can only:

1. Send Command through `CommandQueue`.
2. Subscribe Event through `EventBus`.
3. Subscribe State Snapshot through `StateSnapshotStore`.

Cancellation is represented by `CancellationSource` and `CancellationToken`.

## Current Scope

- No UI layer is implemented.
- No playback hot path is implemented.
- The fake backend can use these primitives to drive SessionCore and PlaybackCore later.

## Gate

`verify_p1_gates.py` checks that Engine API boundary source files exist and are wired into CMake.


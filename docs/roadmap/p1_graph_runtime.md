# P1 Graph Runtime

`FakeGraphRuntime` proves the GraphCore execution boundary without real demux, decode, render, or audio nodes.

## Covered Shape

- graph binding with generation
- fake nodes with lifecycle state
- fake edges with bounded capacity
- backpressure marker
- seek barrier generation
- flush generation
- stop state

## Invariants

- Every edge has capacity.
- Every node has lifecycle state.
- Graph stop forbids later frame production.
- Graph flush carries a non-zero generation.

## Boundary

GraphCore does not perform source ranking, policy decisions, quality tier selection, HDR/DV fallback decisions, audio passthrough policy, subtitle language policy, or user-visible fallback copy.


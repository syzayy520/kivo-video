# P0 Core Freeze Planning

## Task type

planning-first / contract-only / no-real-playback / no-adapter-runtime / no-ui

## P0 target

P0 freezes the foundation contracts only: schema versioning, trace identity, contract registry, state ownership, event ordering, source identity, playback plan, capability join, decision ledger, color output, Windows HDR output, presenter contract, audio/subtitle contracts, diagnostic manifest, and governance gates.

P0 does not implement real FFmpeg decode, real D3D presentation, real WASAPI output, provider SDK access, or UI behavior.

## P0 order

1. Governance and iron gate
2. Foundation / schema / trace
3. Contract registry
4. Source / remote identity
5. Playback plan
6. Session / decision ledger
7. Capability join
8. Media experience snapshot
9. Color / Windows HDR / presenter contracts
10. Audio / subtitle contracts
11. Diagnostic / inspector contracts
12. Minimal CI gates

## Single-family rule

Each implementation round may touch only one natural family. This first round is limited to:

- foundation/schema
- foundation/trace
- foundation/contract
- source/remote_identity

It must not implement playback runtime, graph runtime, decoder, renderer, audio output, subtitle renderer, or third-party adapters.

## Acceptance

- Folders represent natural families.
- Every file has one responsibility.
- Contract Registry can register contract entries.
- RemoteObjectIdentity can express object identity, Range behavior, DirectLink lifetime, refresh strategy, and cache identity key.
- CMake can configure the foundation library and the remote identity contract test.
- Business core must not include native Windows, FFmpeg, D3D, DXGI, WASAPI, or provider SDK headers.

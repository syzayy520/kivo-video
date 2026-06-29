# KIVO-VIDEO-P7A-PLANNING-DRAFT-001

**Document type**: planning-draft / natural-family-tree / implementation authorization base
**Stage**: P7A
**Status**: `READY_FOR_P7B_AFTER_P7A_006`
**Generated**: 2026-06-29
**Implementation baseline**: `.codex/Kivo Video P7 Playback Graph Integration Design V1.9.1.docx`
**Baseline hash**: `9A52A8B2C14F08F5D804771759B32520B991D7D19963A0DE49B238505D4ED05F`

## 1. Iron Gate Compliance

| Gate | Result |
|---|---|
| Conception | P7 is Playback Graph Orchestration Runtime only. |
| Planning design | This document plus `p7_v1.9.1_execution_card.md`. |
| Natural family tree | Defined below before implementation. |
| Implementation | Allowed only after `P7A-006`. |
| Single file, single responsibility | Every proposed file has one responsibility. |
| No flat files | All files live under semantic P7 families. |

## 2. Baseline Decision

Authoritative implementation source:

```text
C:\kivo video\.codex\Kivo Video P7 Playback Graph Integration Design V1.9.1.docx
```

Recovered historical carryover source:

```text
C:\Users\huyasi\Downloads\Kivo Video P7 Playback Graph Integration Design V1.9.docx
```

The V1.9 document is a historical audit source only. P7 implementation remains locked to V1.9.1.
The temporary baseline replacement decision is superseded by the real V1.9 recovery.

Evidence:

```text
artifacts/p7/evidence/P7A-003.json
artifacts/p7/evidence/P7A-003R.json
docs/roadmap/p7a_baseline_replacement_decision.md
docs/roadmap/p7a_v1.9_to_v1.9.1_carryover_report.md
docs/design/p7/KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.md
docs/design/p7/KIVO-VIDEO-P7-PLAYBACK-GRAPH-INTEGRATION-DESIGN-LOCK-001-V1.9.rule_manifest.yaml
```

## 3. Dependency Inventory Summary

| Area | Current root | Inventory result | P7 usage |
|---|---|---:|---|
| P3/P4 legacy cinema engine | `backend/include/kivo/cinema_engine` | 276 headers / 51 dirs | P4 state, command, source/session adapter input |
| P5 video plane | `include/kivo/video_plane` | 219 headers / 44 dirs | video attachment, queue, render/present, protected policy, evidence input |
| P6 audio public contracts | `include/kivo/video/audio_plane` | 320 headers / 40 dirs | audio clock, output, capability, render, evidence, command input |
| P6 private runtime | `backend/include_private/video/audio_plane` | 21 headers / 10 dirs | private real runtime only; P7 public API must not include it |
| P6 runtime source | `backend/src/video/audio_plane` | 30 files / 9 dirs | real runtime implementation; P7 must coordinate, not rewrite |

## 4. Gap Classification

| Gap | Classification | Blocks minimal fake E2E | Blocks real backend | P7 handling |
|---|---|---:|---:|---|
| V1.9 historical carryover audit has manual-review rows | NonBlockingAuditGap | No | No | Use V1.9.1 as implementation baseline; resolve 32 manual-review rows before final P7 closure |
| P4 has `std::string command_id`, no typed `PlaybackCommandId` | SoftBlockingGap | No | Yes | P7 owns typed command ids and maps through `P4StateSyncAdapter` |
| P4 has no typed `PlaybackRevision` | SoftBlockingGap | No | Yes | P7 owns public revision type and uses fake adapter; real semantic ownership remains adapter evidence |
| Real P5/P6 async completion shape varies | SoftBlockingGap | No | Yes | P7 defines `OperationKey`/completion contracts and fake attachments first |
| Product P10 performance SLA absent | DeferredProductGap | No | No | P7 reports fake integration budget only |

## 5. Natural Family Tree

### 5.1 Public Contract Family

```text
include/kivo/video/playback_graph/
  all_public_headers.hpp
  p7_api_version.hpp
  playback_session.hpp
  playback_session_snapshot.hpp
  playback_graph_state.hpp
  playback_graph_error.hpp
  playback_graph_ids.hpp
  command_token.hpp
  command_lifecycle_snapshot.hpp
  seek_request.hpp
  track_switch_request.hpp
  playback_graph_event.hpp
  playback_graph_observer.hpp
  generation_stamp.hpp
  snapshot_query_result.hpp
  playback_graph_policy.hpp
```

### 5.2 Private Runtime Family

```text
backend/include_private/video/playback_graph/
  session/playback_session_runtime.hpp
  state/playback_graph_state_machine.hpp
  command/command_lifetime_registry.hpp
  command/command_conflict_matrix.hpp
  command/pending_transport_intent.hpp
  p4_bridge/p4_state_sync_adapter.hpp
  attachment/attachment_operation_registry.hpp
  attachment/phase_bitset.hpp
  generation/generation_stamp_publisher.hpp
  events/critical_event_mpsc_ring.hpp
  realtime/realtime_event_allocator.hpp
  coordination/seek_coordinator.hpp
  coordination/track_switch_coordinator.hpp
  coordination/eos_drain_coordinator.hpp
  recovery/recovery_replay_coordinator.hpp
  resources/resource_retire_recycle.hpp
  snapshot/snapshot_store.hpp
  observer/observer_registry.hpp
  evidence/critical_proof_ring.hpp
  metrics/graph_metrics_budget.hpp
  health/graph_health_monitor.hpp
  internal/hot_path_contract_guard.hpp
```

### 5.3 Runtime Source Family

```text
backend/src/video/playback_graph/
  session/playback_session_facade.cpp
  session/playback_session_runtime.cpp
  state/playback_graph_state_machine.cpp
  command/command_lifetime_registry.cpp
  command/command_conflict_matrix.cpp
  p4_bridge/p4_state_sync_adapter.cpp
  attachment/attachment_operation_registry.cpp
  generation/generation_stamp_publisher.cpp
  events/critical_event_mpsc_ring.cpp
  coordination/seek_coordinator.cpp
  coordination/track_switch_coordinator.cpp
  coordination/eos_drain_coordinator.cpp
  recovery/recovery_replay_coordinator.cpp
  resources/resource_retire_recycle.cpp
  snapshot/snapshot_store.cpp
  observer/observer_registry.cpp
  evidence/critical_proof_ring.cpp
  metrics/graph_metrics_budget.cpp
  health/graph_health_monitor.cpp
```

### 5.4 Test and Tooling Family

```text
backend/tests/video/playback_graph/
  p7b_public_contract_tests.cpp
  p7c_command_state_tests.cpp
  p7d_async_generation_tests.cpp
  p7e_behavior_coordinator_tests.cpp
  p7f_snapshot_evidence_tests.cpp
  p7g_tooling_guard_tests.cpp
  test_support/fake_clock.hpp
  test_support/manual_event_pump.hpp
  test_support/fake_attachments.hpp

backend/tools/video/playback_graph/
  architecture_guard/p7_arch_guard.py
  contract_gap_scanner/p7_contract_gap_scanner.py
  contract_header_diff/p7_contract_header_diff.py
  matrix_codegen/p7_matrix_codegen.py
  contract_manifest.yaml
```

## 6. Per-File Responsibility Table

| File | Owner gate | Single responsibility |
|---|---|---|
| `all_public_headers.hpp` | P7B-001 | Include every P7 public header for purist compile checks. |
| `p7_api_version.hpp` | P7B-001 | Define P7 API version constants. |
| `playback_session.hpp` | P7B-002 | Declare the public PlaybackSession facade. |
| `playback_session_snapshot.hpp` | P7B-006 | Define bounded public session snapshot value. |
| `playback_graph_state.hpp` | P7B-004 | Define graph states only. |
| `playback_graph_error.hpp` | P7B-004 | Define layered public errors, evidence reasons, guard failures, severity. |
| `playback_graph_ids.hpp` | P7B-003 | Define strong ids and revisions. |
| `command_token.hpp` | P7B-003 | Define nonblocking command token value. |
| `command_lifecycle_snapshot.hpp` | P7B-003 | Define queryable command lifecycle snapshot. |
| `seek_request.hpp` | P7B-005 | Define seek request and seek mode values. |
| `track_switch_request.hpp` | P7B-005 | Define audio/video/AV track switch request values. |
| `playback_graph_event.hpp` | P7B-005 | Define public graph event values. |
| `playback_graph_observer.hpp` | P7B-006 | Define observer handle and subscription token ABI. |
| `generation_stamp.hpp` | P7B-004 | Define public generation stamp value and state. |
| `snapshot_query_result.hpp` | P7F-001 | Define typed snapshot query result and errors. |
| `playback_graph_policy.hpp` | P7F-004 | Define public policy knobs and timeout/budget fields. |
| `playback_session_runtime.hpp/.cpp` | P7C-001 | Own private session runtime composition. |
| `playback_session_facade.cpp` | P7C-001 | Own public `PlaybackSession` PIMPL allocation, move, destruction, and delegation only. |
| `playback_graph_state_machine.hpp/.cpp` | P7C-002 | Validate and apply graph state transitions. |
| `command_lifetime_registry.hpp/.cpp` | P7C-006 | Own command records, retention, tombstones. |
| `command_conflict_matrix.hpp/.cpp` | P7C-007 | Resolve command/state/policy conflict decisions. |
| `pending_transport_intent.hpp` | P7C-008 | Collapse pause/resume pending intent. |
| `p4_state_sync_adapter.hpp/.cpp` | P7C-005 | Isolate legacy P4 command/revision boundary. |
| `attachment_operation_registry.hpp/.cpp` | P7D-001 | Own OperationKey, tokens, completion/tombstone records. |
| `phase_bitset.hpp` | P7D-004 | Track up to 64 required async operations. |
| `generation_stamp_publisher.hpp/.cpp` | P7D-005 | Publish/read generation stamp via uint64 seqlock. |
| `critical_event_mpsc_ring.hpp/.cpp` | P7D-006 | Provide bounded critical event MPSC ring. |
| `realtime_event_allocator.hpp` | P7D-007 | Provide fixed-slot realtime allocation contract. |
| `hot_path_contract_guard.hpp` | P7D-008 | Centralize debug/static hot path guard markers. |
| `seek_coordinator.hpp/.cpp` | P7E-002 | Coordinate seek phases and atomic commit. |
| `track_switch_coordinator.hpp/.cpp` | P7E-005 | Coordinate track switch transactions. |
| `eos_drain_coordinator.hpp/.cpp` | P7E-004 | Coordinate EOS drain phase bitsets. |
| `recovery_replay_coordinator.hpp/.cpp` | P7E-006 | Coordinate recovery replay transactions. |
| `resource_retire_recycle.hpp/.cpp` | P7E-003 | Own retire/recycle barrier state and evidence. |
| `snapshot_store.hpp/.cpp` | P7F-001 | Store immutable snapshots and typed query results. |
| `observer_registry.hpp/.cpp` | P7F-002 | Manage observer lifecycle and delivery policy. |
| `critical_proof_ring.hpp/.cpp` | P7F-003 | Preserve in-memory critical proof if sink fails. |
| `graph_metrics_budget.hpp/.cpp` | P7F-004 | Validate queue/memory/timeout budget policy. |
| `graph_health_monitor.hpp/.cpp` | P7F-004 | Sample graph health and stalled predicates. |
| `p7_arch_guard.py` | P7G-004 | Enforce P7 public/private forbidden dependency rules. |
| `p7_contract_gap_scanner.py` | P7G-002 | Classify P7 contract gaps. |
| `p7_contract_header_diff.py` | P7G-003 | Compare manifest against header surfaces. |
| `p7_matrix_codegen.py` | P7G-005 | Generate matrix outputs from one source. |
| `contract_manifest.yaml` | P7G-001 | List P7 dependency contracts and expected surfaces. |

## 7. Allowed Dependencies

| P7 area | Allowed |
|---|---|
| Public include | C++ standard library value types, fixed-size buffers, P7 public headers only |
| Private runtime | P7 private headers, P7 public headers, legacy P4 headers through adapter only, P5/P6 public contracts through adapter/coordinator only |
| Tests | P7 test support, fake attachments, fake clock, manual event pump |
| Tooling | Python standard library and project paths |

## 8. Forbidden Dependencies

| P7 area | Forbidden |
|---|---|
| Public include | Qt, FFmpeg, D3D/DXGI, WASAPI, provider APIs, subtitle runtime, backend private headers, test support |
| Private runtime | real decoder implementation ownership, real renderer implementation ownership, real audio output implementation ownership, provider redesign, UI |
| Tests | hidden dependency on real hardware for fake integration tests |
| Tooling | rewriting product source as part of scans |

## 9. No-Omission Matrix

| Design sections | Planned coverage |
|---|---|
| 0..5 Scope and API boundary | P7A, P7B, P7G |
| 6..14 Public commands, transactions, state, P4 bridge, track switch | P7B, P7C, P7E |
| 15..20 async attachment, timeout, generation, critical ring, hot path | P7D |
| 21..23 snapshot, observer, budgets, buffering/stalled | P7F, P7E |
| 24..30 seek, resources, EOS, recovery, AV sync, system resume, protected content | P7E |
| 31..32 evidence, metrics, privacy, error taxonomy | P7F |
| 33..37 contract gaps, manifest, architecture guard, tests, stop conditions | P7G |
| 38 G1-G60 exit criteria | P7H matrix, with stage evidence |
| 39 final report | P7H |
| 40 non-goals | P7H redline scan |
| 41 delivery statement | P7H final closure |

## 10. P7B Start Conditions

P7B may start only when:

- `P7A-001.json`, `P7A-002.json`, `P7A-003.json`, `P7A-004.json`, `P7A-005.json`, and `P7A-006.json` exist.
- `P7A-003.json` confirms the recovered V1.9 baseline files and carryover audit seed.
- `P7A-003R.json` remains preserved only as superseded temporary recovery history.
- No implementation file has been created before `P7A-006`.
- The user instruction authorizes professional continuation through all P7 gates.

**END OF P7A PLANNING DRAFT**

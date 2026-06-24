# P2 Natural Family Tree

Updated: 2026-06-25

## Design Principles

1. Each family owns one semantic responsibility
2. Families are organized by domain, not by technology
3. Adapter modules sit at the boundary, not in the core
4. Test files mirror production structure

## Family Tree

```
backend/
├── include/kivo/cinema_engine/
│   │
│   │── foundation/
│   │   └── contract_registry.hpp
│   │
│   │── engine_api/
│   │   ├── command_queue.hpp
│   │   ├── event_bus.hpp
│   │   ├── state_snapshot_store.hpp
│   │   ├── cancellation_token.hpp
│   │   ├── engine_command.hpp
│   │   ├── engine_event.hpp
│   │   ├── engine_error.hpp
│   │   └── engine_state_snapshot.hpp
│   │
│   │── session_core/
│   │   ├── playback_session_aggregate.hpp
│   │   └── session_store.hpp
│   │
│   │── decision_ledger_core/
│   │   ├── decision_ledger_entry.hpp
│   │   ├── decision_ledger_store.hpp
│   │   └── replay_manifest.hpp
│   │
│   │── playback_core/
│   │   ├── playback_state.hpp
│   │   ├── playback_plan_schema.hpp
│   │   ├── playback_invariant.hpp
│   │   └── playback_state_machine.hpp
│   │
│   │── source_core/
│   │   ├── source_playable_object.hpp
│   │   ├── remote_object_identity.hpp
│   │   ├── source_adapter_contract.hpp      [NEW P2]
│   │   └── source_health_snapshot.hpp        [NEW P2]
│   │
│   │── network_cache_core/
│   │   ├── cache_reader.hpp
│   │   └── segment_cache_runtime.hpp         [NEW P2]
│   │
│   │── content_integrity_core/
│   │   └── content_integrity_contract.hpp    [NEW P2]
│   │
│   │── demux_core/
│   │   ├── stream_inventory.hpp
│   │   └── demux_runtime.hpp                 [NEW P2]
│   │
│   │── decode_core/
│   │   ├── decode_path.hpp
│   │   └── frame_output.hpp                  [NEW P2]
│   │
│   │── graph_core/
│   │   ├── graph_runtime.hpp
│   │   ├── graph_invariant.hpp
│   │   ├── graph_node.hpp                    [NEW P2]
│   │   └── graph_edge.hpp                    [NEW P2]
│   │
│   │── media_timeline_core/
│   │   ├── timebase_normalization.hpp        [NEW P2]
│   │   ├── master_clock.hpp                  [NEW P2]
│   │   └── av_sync_policy.hpp                [NEW P2]
│   │
│   │── presentation_timing_core/
│   │   ├── frame_pacing.hpp                  [NEW P2]
│   │   └── vsync_policy.hpp                  [NEW P2]
│   │
│   │── video_render_core/
│   │   ├── video_presenter_contract.hpp
│   │   ├── presenter_service.hpp
│   │   └── d3d11_presenter.hpp               [NEW P2]
│   │
│   │── color_science_core/
│   │   ├── color_output_matrix.hpp
│   │   └── hdr_detection.hpp                 [NEW P2]
│   │
│   │── audio_core/
│   │   ├── audio_output_decision.hpp
│   │   ├── fake_audio_service.hpp
│   │   └── audio_endpoint_contract.hpp               [NEW P2]
│   │
│   │── subtitle_core/
│   │   ├── subtitle_render_plan.hpp
│   │   ├── fake_subtitle_service.hpp
│   │   └── subtitle_overlay.hpp              [NEW P2]
│   │
│   │── capability_core/
│   │   ├── capability_join_result.hpp
│   │   ├── static_capability_snapshot.hpp    [NEW P2]
│   │   ├── device_capability_snapshot.hpp    [NEW P2]
│   │   ├── session_capability_snapshot.hpp   [NEW P2]
│   │   └── runtime_capability_observation.hpp [NEW P2]
│   │
│   │── policy_core/
│   │   ├── policy_decision.hpp
│   │   └── playback_eligibility.hpp          [NEW P2]
│   │
│   │── compliance_core/
│   │   └── compliance_gate_result.hpp
│   │
│   │── feature_gate_core/
│   │   └── feature_gate_snapshot.hpp
│   │
│   │── resource_core/
│   │   ├── resource_budget_result.hpp
│   │   └── memory_budget.hpp                 [NEW P2]
│   │
│   │── playback_inspector_core/
│   │   ├── playback_inspector_snapshot.hpp
│   │   └── inspector_runtime.hpp             [NEW P2]
│   │
│   │── diagnostic_core/
│   │   ├── diagnostic_bundle_manifest.hpp
│   │   ├── diagnostic_redaction_gate.hpp
│   │   └── telemetry_ring_buffer.hpp         [NEW P2]
│   │
│   │── storage_core/
│   │   ├── source_db.hpp                     [NEW P2]
│   │   ├── library_db.hpp                    [NEW P2]
│   │   ├── playback_db.hpp                   [NEW P2]
│   │   ├── decision_db.hpp                   [NEW P2]
│   │   ├── cache_db.hpp                      [NEW P2]
│   │   └── telemetry_db.hpp                  [NEW P2]
│   │
│   │── security_core/
│   │   ├── credential_policy.hpp             [NEW P2]
│   │   └── redirect_policy.hpp               [NEW P2]
│   │
│   │── governance_core/
│   │   ├── schema_migration_policy.hpp       [NEW P2]
│   │   ├── claim_vocabulary_gate.hpp         [NEW P2]
│   │   └── release_readiness_matrix.hpp      [NEW P2]
│   │
│   └── adapter_boundary/
│       └── adapter_boundary_contract.hpp
│
├── src/
│   └── (mirrors include/ structure, with .cpp files)
│
└── tests/
    └── (mirrors src/ structure, with _test.cpp files)
```

## P2 New Families Summary

| Family | Responsibility | P2 Phase |
|--------|---------------|----------|
| source_core/source_adapter_contract | Source adapter interface | P2.2 |
| source_core/source_health_snapshot | Remote source health | P2.2 |
| content_integrity_core | Content integrity check | P2.1 |
| demux_core/demux_runtime | Real FFmpeg demux | P2.1 |
| decode_core/frame_output | Decoded frame output | P2.1 |
| graph_core/graph_node | Graph node lifecycle | P2.1 |
| graph_core/graph_edge | Graph edge capacity | P2.1 |
| media_timeline_core/timebase | Time normalization | P2.1 |
| media_timeline_core/master_clock | Audio-master clock | P2.1 |
| media_timeline_core/av_sync | AV sync policy | P2.1 |
| presentation_timing_core | Frame pacing, VSync | P2.1 |
| video_render_core/d3d11_presenter | D3D11 presenter | P2.1 |
| color_science_core/hdr_detection | HDR/DV detection | P2.3 |
| audio_core/audio_endpoint_contract | Audio endpoint contract | P2.1 |
| subtitle_core/subtitle_overlay | D3D11 subtitle overlay | P2.3 |
| capability_core/*_snapshot | Capability layers | P2.1 |
| policy_core/playback_eligibility | Eligibility check | P2.1 |
| resource_core/memory_budget | Memory budget | P2.1 |
| playback_inspector_core/inspector_runtime | Inspector runtime | P2.3 |
| diagnostic_core/telemetry_ring_buffer | Ring buffer | P2.3 |
| storage_core/*_db | SQLite domains | P2.1 |
| security_core | Credential, redirect | P2.2 |
| governance_core | Migration, claims, release | P2.3 |

# P2 Remaining Coverage Natural Family Tree

Updated: 2026-06-26

This document maps all P2 remaining families to the existing backend natural family tree style, following the iron gate rule: conception -> planning design -> natural family tree -> implementation.

## Backend Family Mapping

### Foundation Families (Already Exist)

- `backend/include/kivo/cinema_engine/foundation`: schema metadata, contract registry, shared lifecycle contracts.
- `backend/include/kivo/cinema_engine/engine_api`: command, event, snapshot boundary for UI and callers.
- `backend/include/kivo/cinema_engine/session_core`: session identity and aggregate state.
- `backend/include/kivo/cinema_engine/decision_ledger_core`: replay-critical decision chain.
- `backend/include/kivo/cinema_engine/playback_core`: playback state machine and generation contracts.
- `backend/include/kivo/cinema_engine/playback_plan_core`: selected source, track, fallback, and recovery plan.
- `backend/include/kivo/cinema_engine/source_core`: source playable object contract.
- `backend/include/kivo/cinema_engine/remote_object_core`: stable remote object identity.
- `backend/include/kivo/cinema_engine/capability_core`: layered capability join.
- `backend/include/kivo/cinema_engine/policy_core`: policy decisions.
- `backend/include/kivo/cinema_engine/compliance_core`: claim and fail-closed decisions.
- `backend/include/kivo/cinema_engine/feature_gate_core`: snapshotized feature gates.
- `backend/include/kivo/cinema_engine/media_experience_core`: user-visible media experience and badges.
- `backend/include/kivo/cinema_engine/color_science_core`: HDR and DV-aware output matrix.
- `backend/include/kivo/cinema_engine/windows_hdr_core`: Windows HDR output contract.
- `backend/include/kivo/cinema_engine/video_render_core`: presenter and frame pacing contract.
- `backend/include/kivo/cinema_engine/subtitle_core`: subtitle rendering plan.
- `backend/include/kivo/cinema_engine/audio_core`: audio output decision.
- `backend/include/kivo/cinema_engine/playback_inspector_core`: user and developer inspection snapshot.
- `backend/include/kivo/cinema_engine/diagnostic_core`: diagnostic bundle and redaction boundary.
- `backend/include/kivo/cinema_engine/graph_core`: graph runtime execution state.
- `backend/include/kivo/cinema_engine/network_cache_core`: cache extent identity boundary.

### P2 Remaining Families (New Mappings)

#### 1. Local Repair Delta Family (`local_repair_delta`)
- **Task IDs**: LRD-001 through LRD-013
- **Responsibility**: Local playback proof, device reality audit, presenter boundary, PCM conversion, WASAPI shared mode, E2E runtime, micro soak, evidence artifacts, final report consistency, regression protection.
- **Directory Mapping**: No new directory - uses existing families:
  - `backend/include/kivo/cinema_engine/video_upload/` (D3D11 device, frame upload)
  - `backend/include/kivo/cinema_engine/presenter/` (presenter boundary)
  - `backend/include/kivo/cinema_engine/audio_output/` (PCM conversion, WASAPI)
  - `backend/include/kivo/cinema_engine/playback_core/` (E2E runtime, clock, seek)
  - `backend/include/kivo/cinema_engine/playback_inspector_core/` (inspector snapshots)
- **Dependency Direction**: LRD-001 → LRD-013 (sequential)
- **Status**: Planning phase (LRD-001 pending)

#### 2. Evidence Schema Family (`evidence_schema`)
- **Task IDs**: SGF-001 through SGF-008
- **Responsibility**: Evidence schema validation, artifact path enforcement, schema versioning, privacy compliance, anti-fake validation.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/governance_core/`
- **Dependency Direction**: SGF-001 → SGF-008 (sequential)
- **Status**: Queued after LRD-013

#### 3. Redline Gate Family (`redline_gate`)
- **Task IDs**: RGF-001 through RGF-005
- **Responsibility**: Redline gate enforcement, forbidden claim scanning, report/source conflict detection, premature claim prevention.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/governance_core/`
- **Dependency Direction**: RGF-001 → RGF-005 (sequential)
- **Status**: Queued after SGF-008

#### 4. Format Compatibility Family (`format_compat`)
- **Task IDs**: FMT-001
- **Responsibility**: Format compatibility matrix, codec support, container format validation, transcoding policy.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/decode_core/` (existing)
- **Dependency Direction**: After RGF-005
- **Status**: Queued

#### 5. Media Identity Family (`media_identity`)
- **Task IDs**: MID-001
- **Responsibility**: Media identity resolution, fingerprinting, metadata extraction, content identification.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/source_core/` (existing)
- **Dependency Direction**: After FMT-001
- **Status**: Queued

#### 6. Subtitle and Chapter Family (`subtitle_chapter`)
- **Task IDs**: SUB-001
- **Responsibility**: Subtitle rendering, chapter navigation, subtitle format support, chapter metadata.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/subtitle_core/` (existing)
- **Dependency Direction**: After MID-001
- **Status**: Queued

#### 7. Source Core Family (`source_core`)
- **Task IDs**: SRC-001
- **Responsibility**: Source playable object contracts, source identity, source location, source access.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/source_core/` (existing)
- **Dependency Direction**: After SUB-001
- **Status**: Queued

#### 8. Provider Runtime Family (`provider_runtime`)
- **Task IDs**: PMX-001, PMX-002
- **Responsibility**: Provider runtime matrix generation, provider honesty gate, provider status tracking.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/governance_core/`
- **Dependency Direction**: PMX-001 → PMX-002
- **Status**: Queued after foundation families

#### 9. Playback State Family (`playback_state`)
- **Task IDs**: PST-001
- **Responsibility**: Playback state management, state transitions, state persistence, state recovery.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/playback_core/` (existing)
- **Dependency Direction**: After provider runtime
- **Status**: Queued

#### 10. Audio Codec Policy Family (`audio_codec_policy`)
- **Task IDs**: ACP-001
- **Responsibility**: Audio codec policy, codec selection, bitrate policy, channel mapping.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/audio_core/` (existing)
- **Dependency Direction**: After PST-001
- **Status**: Queued

#### 11. Presentation Extension Family (`presentation_extension`)
- **Task IDs**: CAST-001, PIP-001, CIN-001, TRK-001
- **Responsibility**: Cast foundation, PiP foundation, cinema foundation, Trakt foundation.
- **Directory Mapping**: 
  - `backend/include/kivo/cinema_engine/integration_core/` (existing)
  - `backend/include/kivo/cinema_engine/media_experience_core/` (existing)
- **Dependency Direction**: After ACP-001
- **Status**: Queued

#### 12. Final Gate Family (`final_gate`)
- **Task IDs**: PFFG-001, PFFG-002, APRV-001
- **Responsibility**: P2 foundation final gate, foundation final report, all providers runtime verified gate.
- **Directory Mapping**: `backend/include/kivo/cinema_engine/governance_core/`
- **Dependency Direction**: PFFG-001 → PFFG-002 → APRV-001
- **Status**: Final gates after all other families complete

## Dependency Graph

```
V8-001 → V8-002 → V8-003 → V8-004 → V8-005 → V8-006 → V8-007 → V8-008 → V8-009
    ↓
LRD-001 → LRD-002 → ... → LRD-013
    ↓
SGF-001 → ... → SGF-008 → RGF-001 → ... → RGF-005
    ↓
FMT-001 → MID-001 → SUB-001 → SRC-001
    ↓
NET-001 → OBJ-001 → WDA-001 → MSV-001 → CLD-001 → IPTV-001
    ↓
PMX-001 → PMX-002
    ↓
PST-001 → ACP-001 → CAST-001 → PIP-001 → CIN-001 → TRK-001
    ↓
PFFG-001 → PFFG-002 → APRV-001
```

## Provider Families (Additional)

- `NET-001`: Network File Sources
- `OBJ-001`: Object Storage
- `WDA-001`: WebDAV and AList
- `MSV-001`: Media Server
- `CLD-001`: Cloud Drive
- `IPTV-001`: IPTV Live

## Rules

1. No `common`, `misc`, or utility dumping-ground families.
2. Family names must match task IDs (e.g., `local_repair_delta` for LRD tasks).
3. Each family must have clear responsibility boundaries.
4. Existing families are reused where possible.
5. New families only when responsibility doesn't fit existing structure.

## Status

- ✅ V8-004 Natural Family Tree Gate: Mapping complete
- ⏳ Pending implementation phases
- 🔒 No runtime PASS claims yet (planning only)
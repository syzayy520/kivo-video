# P2 Real Playback Architecture

Updated: 2026-06-25

## Overview

P2 transforms KivoCinemaEngine from a fake-backend contract scaffold into a real playback engine. The architecture preserves the Iron Gate boundary: core modules own contracts, adapter modules own platform types.

## Execution Phases

| Phase | Scope | Goal |
|-------|-------|------|
| P2.1 | Local Real Playback Closure | Local MP4/MKV SDR play/pause/seek/resume/stop |
| P2.1.5 | Disc Image / BDMV Closure | BDMV folder and mounted ISO playback |
| P2.2 | Remote Direct Play Closure | WebDAV / AList / Emby verified range playback |
| P2.3 | Quality / Recovery / Inspector Closure | Subtitle runtime, HDR detection, telemetry, governance |

## Module Family Tree (P2)

```
backend/
├── include/kivo/cinema_engine/
│   ├── engine_api/           # Command, Event, Snapshot, Cancellation
│   ├── session_core/         # PlaybackSessionAggregate
│   ├── decision_ledger_core/ # DecisionLedgerEntry, ReplayManifest
│   ├── playback_core/        # PlaybackState, PlaybackPlan
│   ├── source_core/          # SourcePlayableObject, RemoteObjectIdentity
│   ├── network_cache_core/   # SegmentCacheRuntime
│   ├── content_integrity_core/
│   ├── demux_core/           # StreamInventory, DemuxRuntime
│   ├── decode_core/          # DecodePath, FrameOutput
│   ├── graph_core/           # GraphRuntime, Node, Edge
│   ├── media_timeline_core/  # Timebase, MasterClock
│   ├── presentation_timing_core/ # FramePacing, VSync
│   ├── video_render_core/    # Presenter, Upload
│   ├── color_science_core/   # ColorOutputMatrix, HDR
│   ├── audio_core/           # AudioOutputDecision, Endpoint
│   ├── subtitle_core/        # SubtitleRenderPlan, Overlay
│   ├── capability_core/      # CapabilityJoinResult
│   ├── policy_core/          # PolicyDecision
│   ├── compliance_core/      # ComplianceGateResult
│   ├── feature_gate_core/    # FeatureGateSnapshot
│   ├── resource_core/        # ResourceBudgetResult
│   ├── playback_inspector_core/ # InspectorSnapshot
│   ├── diagnostic_core/      # DiagnosticBundle
│   ├── storage_core/         # SQLite domains
│   ├── security_core/        # Credential, Redaction
│   ├── governance_core/      # CI Gates
│   └── adapter_boundary/     # Adapter contract boundary
├── src/
│   ├── foundation/           # ContractRegistry
│   ├── engine_api/
│   ├── session_core/
│   ├── decision_ledger_core/
│   ├── playback_core/
│   ├── source_core/
│   ├── network_cache_core/
│   ├── demux_core/
│   ├── decode_core/
│   ├── graph_core/
│   ├── media_timeline_core/
│   ├── presentation_timing_core/
│   ├── video_render_core/
│   ├── color_science_core/
│   ├── audio_core/
│   ├── subtitle_core/
│   ├── capability_core/
│   ├── policy_core/
│   ├── compliance_core/
│   ├── feature_gate_core/
│   ├── resource_core/
│   ├── playback_inspector_core/
│   ├── diagnostic_core/
│   ├── storage_core/
│   ├── security_core/
│   ├── governance_core/
│   └── adapter_boundary/
└── tests/
    ├── ... (mirrors src/ structure)
```

## Adapter Boundary

Third-party types (FFmpeg, D3D11, DXGI, WASAPI, Win32, COM) are confined to adapter modules. Core modules consume only adapter-produced contracts.

```
Core Modules (no platform types)
    ↓ contracts only
Adapter Boundary (AdapterBoundaryContract)
    ↓ platform calls
Third-Party Libraries (FFmpeg, D3D11, WASAPI, Win32)
```

## Minimal Executable Architecture Slice (P2.1)

The first real playback path:

```
LocalFileSource → ByteStream → FFmpegProbe → StreamInventory
    → FFmpegDemux → KivoPacket → FFmpegDecode → DecodedFrame
    → GraphNode → Queue → D3D11Presenter → SwapChain
    → WASAPIEndpoint → PCM Output
    → PlaybackStateMachine → InspectorSnapshot
```

## Lock Hierarchy (preserved from P1)

| Level | Scope |
|-------|-------|
| 0 | Immutable snapshot (lock-free read) |
| 1 | Session state lock |
| 2 | Playback state lock |
| 3 | Graph state lock |
| 4 | Queue / buffer lock |
| 5 | Adapter backend lock |

## Thread Model

| Thread | Purpose | Restrictions |
|--------|---------|--------------|
| UI/API | Command dispatch | No direct state mutation |
| Playback | State machine, plan evaluation | No network I/O |
| Demux | Packet extraction | No decode |
| Decode | Frame production | No render |
| Video Render | Presenter, upload | No network/source lock |
| Audio Output | PCM/passthrough | No high-level lock |
| Network | Range read, cache | No playback state lock |
| Telemetry | Metrics, ring buffer | No hot-path lock |

## Governance

P2 governance extends P1 gates with:
- Adapter boundary hardening (no platform symbols in core)
- Third-party dependency lock (version, license, CVE)
- Claim vocabulary freeze (HDR/DV/audio badges)
- Parser security / fuzz skeleton
- Schema migration policy
- Database resilience policy

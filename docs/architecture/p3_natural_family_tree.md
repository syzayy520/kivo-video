# P3 Natural Family Tree

Updated: 2026-06-27

## Purpose

This document defines the P3 directory family tree before any implementation files are created.

P3 follows the Kivo iron gate: concept -> planning design -> natural family tree -> implementation -> single file single responsibility -> no flat files.

## P3 Scope

P3 is the production bridge from Source Runtime to Playback-Ready Input System.

P3 is not P2 continuation, playback core, renderer, decoder, audio output, UI, or segment scheduler.

## Family Tree

### Base Layer

```
backend/src/p3/
  execution/           # ExecutionContext, MonotonicDeadline, BudgetAllocator, RetryBudget, CancellationSignal
  security/            # NetworkPolicy, SSRF, DNS rebinding, redirect security, credential, redaction, SecureMemory
  evidence_contracts/  # Evidence schema, ProbeEvidence, FastStartEvidence, ManifestEvidence, PlaylistEvidence
  source_model/        # SourceOriginIdentity, ProviderIdentity, SourceTransportIdentity, MediaDeliveryIdentity, SourceCapability
  platform_contracts/  # PlatformCapabilityHints, DecoderCapabilityRequirement, AudioCapabilityRequirement, SubtitleCapabilityRequirement, BufferRequirement
  library_contracts/   # Library scraper contract boundary (contract-only)
```

### Middle Layer

```
backend/src/p3/
  provider_integration/  # ProviderBinding, ProviderMetadataHint, ProbeSkipPolicy
  providers/             # Fake providers (Emby, Jellyfin, Alist, WebDAV, HTTP, HLS, DASH, playlist)
    emby/
    jellyfin/
    alist/
    webdav/
    http/
    hls/
    dash/
    playlist/
  transports/            # HTTP transport hardening, WebDAV production hardening
  sessions/              # SourceSession, IndependentRangeReader, ManifestSession, PlaylistSession, SessionErrorObserver
  media_delivery/        # Progressive delivery, HLS boundary, DASH boundary, DRM mixed clear
  media_probe/           # MediaProbe process boundary, FastProbe, CriticalConstraintsChecklist, ProbeEvidence model
```

### Decision Layer

```
backend/src/p3/
  playback_decision/   # PlaybackCandidateSet, CandidateRanking, PairwiseCandidateComparison, PlaybackDecisionTrace, RuntimeRevision
  playback_handoff/    # PlaybackHandoffContract, handoff ownership models, PlaybackDecisionRuntime
  runtime_revision/    # RuntimeRevisionResult, storm control, priority/throttle/coalesce
```

### Test / Tool Layer

```
backend/tests/p3/
  integration_harness/  # In-process HTTP server, TCP loopback, controlled clock, fake fixtures
  architecture_guard/   # Dependency linter, forbidden boundary tests
  golden_evidence/      # Sanitized offline golden evidence pack
  closure/              # Closure evidence and report tests

backend/architecture/
  p3_architecture_policy.json
  p3_golden_diff_policy.json
  p3_direct_executable_matrix.json
```

### Contract-Only Boundaries (No Runtime Implementation)

```
backend/src/p3/
  cloud_drive/     # CloudDriveFamily contracts only, ProviderUnsupportedCandidate
  smb_upnp_dlna/   # Boundary contracts only, BoundaryUnsupportedReason
  unsupported/     # FTP, RTSP, RTMP unsupported protocol candidates
  subtitle/        # Subtitle download boundary, style, sync, burn-in decision contracts
  audio/           # Audio track decision hints, passthrough candidate contracts
  drm/             # DRM metadata contracts only, no license/decryption
```

## Forbidden Patterns

- `provider_manager`, `source_manager`
- `god_provider`, `universal_provider`, `one_big_provider`
- Monolithic `p3.cpp` or `provider_all.cpp`
- Flat files in root or broad source directories
- Common/misc/utility dumping-ground families

## Dependency Direction

```
base <- middle <- decision <- test_tool
```

- Base layer depends on nothing within P3.
- Middle layer depends on base only.
- Decision layer depends on base + middle only.
- Test/tool layer may depend on all layers but is not a runtime dependency.
- No layer may depend on renderer/decoder/audio_output/ffmpeg/UI.

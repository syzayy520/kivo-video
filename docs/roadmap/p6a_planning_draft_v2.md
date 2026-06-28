# P6A Planning Draft V2 (Based on V2.0 Design Lock Candidate)

> **Status**: DRAFT — AWAITING USER APPROVAL  
> **Based on**: Kivo Video P6 Audio Media Plane Design Lock Candidate V2.0  
> **Supersedes**: `docs/roadmap/p6a_planning_draft.md` (V1, REJECTED — not aligned with V2.0)  
> **Generated**: 2026-06-28  
> **Rule**: §44 — NO headers, NO CMake, NO source until this draft is APPROVED by user  

---

## §1. Repo Inventory (P6G-001 Results)

### 1.1 Existing include/kivo/ structure

```
include/kivo/
└── video_plane/           # P5 Video Media Plane (219 .hpp files, FROZEN)
    ├── output_boundary/
    │   └── audio_packet_boundary_item.hpp   # 5 fields only (legacy)
    ├── capability/
    │   ├── capability_registry.hpp
    │   └── capability_registry_contract.hpp
    └── ... (30+ P5 subdirectories)
```

**Key gap**: No `include/kivo/video/audio_plane/` directory exists. P6 must create this from scratch.

### 1.2 Existing backend audio (legacy, to be replaced by P6 contracts)

```
backend/include/kivo/cinema_engine/
├── audio_core/
│   ├── audio_endpoint_contract.hpp      # double buffer_duration_ms (not strong typed)
│   ├── audio_output_decision.hpp
│   ├── fake_audio_endpoint_service.hpp
│   └── fake_audio_service.hpp
└── audio_output/
    ├── decoded_audio_frame_converter.hpp
    ├── wasapi_internal.hpp              # WASAPI internal (private)
    └── wasapi_shared_pcm_writer.hpp     # bool return (not kivo::Expected)
```

```
backend/src/
├── audio_core/   # fake service implementations
└── audio_output/ # WASAPI writer implementations
```

**Key gap**: Legacy audio uses `double`, `bool`, `std::string` — V2.0 requires strong value types + `kivo::Expected`.

### 1.3 Rust audio backend (separate project, verified WASAPI output)

Located in separate `kivo-audio-native-decode-pipeline-p0-009` branch (Rust). P6 C++ does NOT depend on Rust backend. Rust backend verified WASAPI output independently.

### 1.4 CMake status

- Root `CMakeLists.txt`: `project(KivoVideo LANGUAGES CXX)`, `add_subdirectory(backend)`
- `backend/CMakeLists.txt`: all targets use `cxx_std_20` (NOT `cxx_std_23`)
- P5 contracts: `kivo_video_plane_contracts` INTERFACE library (links nothing)
- **Key gap**: V2.0 §3.1 requires C++23. P6 must introduce `cxx_std_23` for `kivo_audio_plane_contracts`.

### 1.5 kivo type infrastructure

- **No `kivo::Expected`**, `kivo::Optional`, `kivo::Variant` exist yet
- P5 uses `bool` return + `std::string` error_message pattern
- **Key gap**: V2.0 §3.2 requires `kivo::Expected<T,E>` for public contracts. P6 must create these foundational types.

---

## §2. P4/P5/P8 Boundary Inventory (P6G-002 Results)

### 2.1 P5→P6 boundary (existing)

| Type | Location | Fields | V2.0 Required |
|------|----------|--------|---------------|
| `AudioPacketBoundaryItem` | `video_plane/output_boundary/` | 5 (packet_sequence, media_timestamp, audio_track_id, is_boundary, operator==) | 27 fields per §7 `P5AudioPacketImportView` |

**Gap**: Existing P5 type has 5 fields. V2.0 §7 requires 27 fields including: stream_id, track_id, packet_id, packet_sequence, pts, dts, duration, time_base, decode_order_index, media_time_range, is_key_packet, is_decoder_safe_boundary, is_discontinuity, is_eos, is_flush_marker, codec_id, codec_profile, codec_private_data_ref, side_data_summary, encrypted_or_protected, protected_hint, data_source, ownership_mode, engine_epoch, pipeline_generation, source_evidence_id.

**Resolution**: P6 creates `P5AudioPacketImportView` as a P6-side contract that P5 must populate. P6 does NOT modify P5's existing `AudioPacketBoundaryItem`. P6's import view is a superset that P5 adapters fill.

### 2.2 P4→P6 boundary (to be created by P6)

V2.0 §4 references but no existing types:
- `AudioPipelineInputPlan` — P4 audio pipeline input plan (NOT FOUND in repo)
- `P4VideoTimelineProxy` — P4 video timeline derived from P5 evidence
- `AudioGaplessPrepareNextCommand` / `CancelReserveCommand` / `CommitTransitionCommand`
- `AudioPlaybackRateCommand`
- `AudioScrubCommand`

**Resolution**: P6 creates boundary types in `boundary/` subdirectory. These are P6's view of P4 commands, not P4's own types.

### 2.3 P6→P4 boundary (to be created by P6)

- `P6ToP4AudioEvent`
- `P6RecoverySeekRequest`
- `P6ToP5ClockSnapshot`
- `P6ToP5SyncIndicator`

### 2.4 P6→P8 boundary (to be created by P6)

- `P6ToP8AudioSummary` — verified summary only, P8 reads from `P6CapabilityRegistry`
- `AudioUserVisibleClaim` — claim token mapping

### 2.5 P5 CapabilityRegistry (existing, P6 reuses pattern)

P5 has `capability_registry.hpp` with `CapabilityDomainId` enum. P6 creates its own `P6CapabilityRegistry` following the same pattern but for audio-specific capabilities.

---

## §3. Public/Private Header Surface Plan (P6G-006)

### 3.1 Public include root

```
include/kivo/video/audio_plane/
```

### 3.2 Allowed public subdirectories (30, from §6)

```
boundary/  command/  platform/  output/  packet/  data/
decode/  passthrough/  frame/  render/  generation/  time/
clock/  sync/  seek/  gapless/  format/  conversion/
channel/  mix/  rate/  evidence/  capability/  security/
recovery/  advanced_format/  threading/  test_contract/
terminology/  version/
```

### 3.3 Forbidden public paths

```
wasapi/  ffmpeg/  qt/  win32/  com/
```

### 3.4 Private backend roots (NOT created in P6A — deferred to P6B+)

```
backend/src/video/audio_plane/output/wasapi/internal/
backend/include_private/video/audio_plane/output/wasapi/
backend/src/video/audio_plane/decode/ffmpeg/internal/
backend/include_private/video/audio_plane/decode/ffmpeg/
```

### 3.5 Fake backend roots (created in P6A for tests only)

```
backend/tests/video/audio_plane/fake_output/
backend/tests/video/audio_plane/fake_decode/
backend/tests/video/audio_plane/fake_passthrough/
```

### 3.6 Header classification (§6 required)

Each header classified as one of:
- **Contract** — interface/trait declarations
- **ValueType** — strong typedef wrappers (uint64_t-based IDs, etc.)
- **Policy** — policy structs with configuration fields
- **Enum** — enum class definitions
- **Evidence** — evidence record types
- **Schema** — version/schema declarations
- **Boundary** — cross-plane crossing types
- **Adapter** — backend adapter contracts
- **InternalOnly** — private backend headers (NOT in public include)
- **TestOnly** — test infrastructure headers
- **NegativeFixture** — compile-fail test files

---

## §4. P5 Packet Import Contract Plan (P6G-007/011)

### 4.1 P5AudioPacketImportView (27 fields, §7)

```cpp
struct P5AudioPacketImportView {
    AudioStreamId stream_id;
    AudioTrackId track_id;
    AudioPacketId packet_id;
    AudioPacketSequence packet_sequence;
    AudioPts pts;
    AudioDts dts;
    AudioSampleDuration duration;
    AudioTimeBase time_base;
    AudioDecodeOrderIndex decode_order_index;
    AudioMediaTimeRange media_time_range;
    bool is_key_packet;
    bool is_decoder_safe_boundary;
    bool is_discontinuity;
    bool is_eos;
    bool is_flush_marker;
    AudioCodecId codec_id;
    AudioCodecProfile codec_profile;
    AudioCodecPrivateDataRef codec_private_data_ref;
    AudioPacketSideDataSummary side_data_summary;
    bool encrypted_or_protected;
    ProtectedAudioBoundaryHint protected_hint;
    AudioDataImportSource data_source;
    AudioDataOwnershipMode ownership_mode;
    AudioEngineEpoch engine_epoch;
    AudioPipelineGeneration pipeline_generation;
    AudioEvidenceId source_evidence_id;
};
```

### 4.2 AudioDataImportSource enum (6 values)

```cpp
enum class AudioDataImportSource {
    SyncBorrowedByteView,
    MoveOwnedBlock,
    CopiedIntoP6PacketPool,
    PoolSlotOwnedByP6,
    ExternalOpaque,
    Rejected,
};
```

### 4.3 P5AudioPacketImportResult enum (12 values)

```cpp
enum class P5AudioPacketImportResult {
    AcceptedMoved,
    AcceptedCopied,
    AcceptedPoolSlot,
    DeferredBackpressure,
    RejectedInvalidGeneration,
    RejectedInvalidOwnership,
    RejectedMissingPrivateData,
    RejectedPacketOrderViolation,
    RejectedProtectedPolicy,
    RejectedOversize,
    RejectedUnsupportedCodec,
    SupersededByFlushOrSeek,
};
```

### 4.4 Import validation rules

1. Validate stream_id / track_id / generation / epoch before accepting bytes
2. Validate packet_sequence monotonicity unless discontinuity is explicit
3. Validate codec private data presence before decode init when required
4. Never store P5 raw pointer after import
5. Never expose packet bytes to P8
6. Protected/encrypted packet summary is redacted
7. Rejected packet release responsibility follows ownership contract
8. Deferred packet must not create ambiguous ownership

### 4.5 Packet disorder policy

- Non-monotonic + discontinuity → accept discontinuity path
- Non-monotonic without discontinuity → reject + PacketOrderViolation
- Duplicate packet → reject unless explicit retry marker
- Backward timestamp + monotonic sequence → timestamp discontinuity + evidence

---

## §5. Data Ownership Split Plan (P6G-012)

### 5.1 Three ownership variants (§8)

| Variant | Struct | Lifetime | Thread-crossing |
|---------|--------|----------|-----------------|
| SyncBorrowedByteView | `AudioSyncByteView` | sync-call-only | NO |
| MoveOwnedBlock | `AudioOwnedBlockHandle` | P6-owned after transfer | YES (via move) |
| PoolSlot | `AudioPoolSlotHandle` | P6 pool-owned | YES (via pool) |

### 5.2 Key rules

- `AudioSyncByteView`: only valid during import call; must not be stored/cross thread/enter backend
- `AudioOwnedBlockHandle`: release_fn must be noexcept; must not run on AudioDeviceThread if heavy; P5 must not touch after transfer
- `AudioPoolSlotHandle`: lightweight retire may be wait-free only if proven; heavy retire → non-realtime owner

### 5.3 Rejected/Deferred/Superseded behavior

- Rejected before transfer → original owner releases
- Rejected after move but before decode → P6 releases or retires into pool
- DeferredBackpressure → ownership remains with sender; P6A forbids DeferredMoveOwned
- SupersededByFlushOrSeek → P6 retires by generation discard; heavy release not on AudioDeviceThread

### 5.4 Decode backend async holding rule

- Decode backend may hold only P6-owned pool slot or owned block
- Must NOT hold borrowed span
- Must return/retire on flush/drain/close/supersede
- Hold timeout must be observable by evidence

---

## §6. Packet/Frame Pool Memory Plan (P6G-011/015)

### 6.1 Packet pool

- Allocation phase: stream open / preroll / controlled expansion only
- Hot path allocation: FORBIDDEN
- Hard limits: max_bytes, max_packets, max_duration_ms, max_frames (all policy-defined, hard enforced)
- Render queue capacity: max_duration must NOT be diagnostic-only

### 6.2 Frame pool

- Decoded frame memory must be pool-backed
- Frame move must be noexcept
- Frame destruction on hot path must be noexcept and lightweight
- Frame buffer release must NOT perform blocking I/O or heap free on AudioDeviceThread

### 6.3 Pressure behavior

| Mode | Behavior |
|------|----------|
| Normal | backpressure upstream |
| Live | drop according to live policy |
| Bit-perfect | rebuffer/fail rather than silent drop |
| Passthrough | drop only complete stale encoded access unit/burst if policy allows |

---

## §7. Decode Lifecycle Plan Including TimedOut (P6G-013)

### 7.1 16 states (§11)

```
NotCreated → Created → WaitingCodecPrivateData → Opening → Ready → Decoding
→ Draining / Flushing / Resetting / FormatChanging
→ TimedOut / FailedRecoverable / FailedFatal / Closed / Superseded
```

### 7.2 Key transitions

- Created → WaitingCodecPrivateData (when required private data missing)
- Opening → TimedOut (on timeout)
- Any active → TimedOut (on backend deadline exceeded)
- Any active → Superseded (on newer generation)
- TimedOut → Resetting or FailedFatal (by policy)
- FailedRecoverable → Resetting → Ready (on reset success)

### 7.3 Timeouts

- Normal decode init: 3000ms
- Live/network decode init: 5000ms
- Protected/licensed backend: policy-defined

### 7.4 Cancellation

- Seek/flush/track switch/close may cancel init/decode
- Cancelled backend must release all P6-owned handles
- Superseded backend must not publish frames
- TimedOut backend must not publish frames after deadline

---

## §8. Encoded Passthrough Parse Path Plan (P6G-014)

### 8.1 Backend separation

- `AudioDecodeBackend`: encoded packet → decoded PCM frame
- `EncodedPassthroughParseBackend`: encoded packet → validated encoded access unit → IEC61937 input unit
- Passthrough path must NOT depend on decoded PCM frames

### 8.2 EncodedAccessUnit (owned payload only)

```cpp
struct EncodedAccessUnit {
    AudioCodecId codec_id;
    PassthroughCodecProfile profile;
    AudioPacketId source_packet_id;
    AudioMediaTime media_start;
    AudioSampleDuration encoded_duration;
    EncodedAccessUnitBoundary boundary;
    AudioTimeBase time_base;
    bool sync_frame;
    bool discontinuity;
    EncodedAccessUnitOwnedPayload payload;  // OWNED, never borrowed
    AudioEvidenceId validation_evidence_id;
};
```

### 8.3 EncodedAccessUnitOwnedPayload must be

- P6-owned pool slot, OR
- P6-owned compact encoded block, OR
- backend-private opaque handle with explicit lifetime

### 8.4 Must NOT be

- borrowed ByteSpanView
- AVPacket*
- raw provider buffer
- raw pointer into P5 packet

### 8.5 IEC61937 policy

Each codec policy must link to: standard evidence file, golden fixture id, data-type mapping, burst spacing rule, padding rule, pause burst rule, transport limit rule, fallback condition.

P6A defines policy shape only. P6F adds constants with standard-backed fixture.

---

## §9. Generation/Epoch/Wrap Plan (P6G-017)

### 9.1 Six generation types

```cpp
using AudioEngineEpoch    = uint64_t;  // 0 = invalid/reserved
using AudioPipelineGeneration = uint64_t;
using AudioDeviceGeneration   = uint64_t;
using AudioStreamGeneration   = uint64_t;
using AudioFormatGeneration   = uint64_t;
using AudioClaimGeneration    = uint64_t;
```

### 9.2 Rules

- 0 is invalid/reserved
- Generation increments monotonically inside an engine epoch
- Normal runtime wrap is considered unreachable
- If increment would produce 0 or wrap → trigger `EngineEpochResetRequired` → stop active pipelines → invalidate all snapshots → reset under new AudioEngineEpoch

### 9.3 Correct freshness (equality only)

```
Fresh iff:
  snapshot_engine_epoch    == current_engine_epoch
  snapshot_device_generation == current_device_generation
  snapshot_stream_generation == current_stream_generation
  snapshot_format_generation == current_format_generation
  snapshot_claim_generation  == current_claim_generation
```

### 9.4 Forbidden

```
snapshot_device_generation <= current_device_generation means fresh  // WRONG
```

Cross-epoch comparison is invalid except equality of full (epoch, generation) pair.

---

## §10. Audio PTS Checked Rescale / Wrap Plan (P6G-018)

### 10.1 Rules

- All PTS/DTS/sample position conversion uses checked rational rescale
- int64 multiply overflow must NOT silently wrap
- int128 or checked multiply path required where supported
- If checked path fails → emit `TimestampOverflow` + fail current packet/frame safely
- No long-term float accumulation for media time

### 10.2 Timestamp discontinuity policy

- Backward timestamp + monotonic packet sequence → requires evidence
- PTS wrap must be explicitly detected by timebase-specific policy
- Seek/flush/discontinuity resets reorder and drift windows
- EOS/flush marker must NOT be treated as valid audio sample

---

## §11. PCM Current Audible Time Formula Plan (P6G-019)

### 11.1 Three separate formulas (§20)

**Current audible** (NOT tail):
```
estimated_pcm_audible_now_media_time =
    device_consumed_media_time
  - endpoint_latency
  - hardware_transport_latency
  + codec_delay_trimmed_offset
  + user_sync_offset

valid interval: [estimate - clock_error_bound, estimate + clock_error_bound]
```

**Endpoint tail**:
```
endpoint_tail_media_time = device_consumed_media_time + device_padding_duration
```

**Render queue tail**:
```
render_queue_tail_media_time = endpoint_tail_media_time + render_queue_duration
```

### 11.2 Mode behaviors

| Mode | Behavior |
|------|----------|
| Shared | system mixer/APO uncertainty increases error_bound |
| Exclusive | endpoint format evidence can reduce error_bound |
| Bluetooth | hardware_transport_latency dominates; unknown → low confidence |
| Paused | audible time freezes or follows P4 synthetic timeline |
| Muted | audio clock may retain authority only if pipeline active+stable |
| No audio track | P6 has no audio clock authority |
| Underfill silence | audible media time continuity breaks; evidence required |
| After device reset/QPC anomaly | previous snapshot invalid |

### 11.3 clock_error_bound

- Uncertainty interval, NEVER additive bias

---

## §12. PCM Tail Time Formula Plan

See §11.1 above — endpoint_tail and render_queue_tail are separate formulas.

---

## §13. Passthrough Audible Anchor Plan (P6G-014/019)

### 13.1 Current anchor formula (§21)

```
estimated_passthrough_audible_now_media_time =
    media_time_of_last_consumed_or_current_burst
  + consumed_fraction_of_current_encoded_unit
  - external_decoder_latency
  + user_sync_offset
```

### 13.2 Carrier tail formula

```
passthrough_carrier_tail_media_time =
    media_time_of_last_submitted_burst
  + submitted_burst_duration_not_yet_consumed
```

### 13.3 Confidence levels

| Level | Conditions |
|-------|-----------|
| High | sync frame index known + carrier sample rate known + burst duration known + device padding known + external decoder latency measured/bounded |
| Medium | sync frame index known but endpoint/external decoder latency unknown |
| Low | burst count only |
| Invalid | no reliable sync frame index and no reliable burst count |

### 13.4 TrueHD/MAT

- Requires sync-frame/access-unit mapping
- If mapping unavailable → confidence = Low/Invalid
- Aggressive correction FORBIDDEN under Low/Invalid

---

## §14. Evidence Topology Plan (P6G-028)

### 14.1 Multi-producer lanes (§32)

| Producer | Lane | Type |
|----------|------|------|
| AudioDeviceThread | → EvidenceThread | SPSC hot queue |
| AudioRenderPrepareThread | → EvidenceThread | SPSC or bounded MPSC |
| AudioDecodeThread | → EvidenceThread | SPSC or bounded MPSC |
| AudioControlThread | → EvidenceThread | non-hot bounded mutex queue |
| AudioRecoveryThread | → EvidenceThread | non-hot bounded queue |
| Device notification callback | → ControlThread | compact event forward (NOT direct durable write) |

### 14.2 Forbidden

- Single global SPSC for all producers

### 14.3 ID generation

```
AudioEvidenceId = (origin, engine_instance_id, monotonic_counter)
AudioCorrelationId = (origin, scope, monotonic_counter)
origin: P4 / P6 / Backend / Test
```

- atomic fetch_add relaxed allowed for counters
- 0 reserved invalid
- wrap triggers EngineEpochResetRequired
- NO UUID on hot path

### 14.4 Critical evidence

- ChainRoot evidence cannot be dropped silently
- If full → increment compact critical counter → coalesce only if same correlation+reason → emit storage degradation evidence

---

## §15. Claim Freshness Matrix Plan (P6G-029)

### 15.1 Dependencies (14, §31)

```
engine_epoch
device_generation
stream_generation
format_generation
output_mode_generation
volume_policy_generation
software_path_generation
ducking_generation
apo_enhancement_generation
passthrough_policy_generation
bitperfect_policy_generation
license_generation
claim_generation
evidence_ttl
```

### 15.2 Fresh rule

A user-visible claim is Fresh ONLY if ALL relevant generations match current.

### 15.3 Stale examples

- device unchanged but shared/exclusive switched → claim stale
- device unchanged but endpoint volume changed → claim stale/degraded
- format unchanged but ReplayGain enabled → bit-perfect claim broken
- license state changed → Dolby/DTS/Atmos claim stale
- APO/enhancement unknown → no positive bit-perfect
- Ducking unknown → no positive bit-perfect

---

## §16. Bit-perfect Strict Gate Plan (P6G-029)

### 16.1 Positive bit-perfect requires ALL (14+ conditions, §30)

1. Exclusive mode active
2. Exact endpoint format or verified valid-bit-preserving mapping
3. No P6 mutating stage
4. No resampler
5. No DSP
6. No DRC/dialnorm
7. No ReplayGain/loudness normalization
8. No software volume
9. No limiter
10. No time-stretch
11. No channel remap/downmix/upmix
12. No silence fill after claim
13. SoftwarePathVerificationStatus >= EndpointSubmissionVerified
14. Endpoint/session volume unchanged or proven non-mutating
15. EndpointVolumeConfidence == Lab VerifiedHardwareVolume OR volume path bypassed/fixed unity with evidence
16. System APO/enhancement impossible or proven bypassed
17. Ducking disabled or proven inactive/non-mutating
18. Claim freshness matrix valid
19. Evidence not expired

### 16.2 NOT enough for positive claim

- UnknownOrSoftwarePossiblyHeuristic EndpointVolume
- TrustedEndpointApi alone
- APOUnknown
- DuckingUnknown
- SessionVolumeUnknown
- Shared mode
- Fake test only
- Stale claim generation
- Device generation mismatch

### 16.3 Claim grades

```
NotClaimable / Eligible / ActiveUnverified / ActiveMeasured / ActiveVerified / BrokenAfterStart
```

- ActiveVerified requires strict gate
- ActiveMeasured requires current-device measurement but cannot bypass Unknown
- ActiveUnverified cannot render as positive "bit-perfect"
- Passthrough cannot render as PCM bit-perfect
- Playback rate != 1.0 disables PCM bit-perfect positive claim

---

## §17. Gapless Reserve P4/P6 Command Plan (P6G-022)

### 17.1 Three P4 commands (§25)

- `AudioGaplessPrepareNextCommand`
- `AudioGaplessCancelReserveCommand`
- `AudioGaplessCommitTransitionCommand`

### 17.2 Prepare command fields

- current_track_id
- next_track_id
- physical_continuity_group_id
- expected_transition_media_time
- reserve_policy
- codec_private_data_ref
- P5 next packet boundary availability

### 17.3 P6 behavior

- P6 may predecode next-track audio only after P4 prepare command
- P6 cannot choose next track
- P6 cannot fetch source data
- P5/P4 must provide next packet boundaries
- P6 owns sample trim / padding continuity evidence

### 17.4 Default reserve

- target = 500ms decoded audio
- minimum = 100ms
- maximum = 2000ms
- memory budget overrides duration

---

## §18. DSP Dither/Quantize/Limiter Order Plan (P6G-024/026)

### 18.1 18-step mandatory PCM processing order (§27)

1. Decode output / source PCM
2. Codec delay / priming / preroll trim
3. DRC / dialnorm if policy enabled
4. Channel order normalization
5. Dialog enhancement if policy enabled
6. Primary + secondary / AD mix if enabled
7. Channel remap
8. Downmix / upmix
9. ReplayGain / loudness normalization if enabled
10. User software volume / mute if enabled
11. Pre-limiter if policy requires
12. Resampler if required
13. Time-stretch / rate transform if required
14. Final safety limiter if policy requires
15. Sample format conversion
16. Dither / noise shaping if bit-depth reduction requires it
17. Quantize / bit-depth reduction
18. Endpoint submission

### 18.2 Key rules

- Dither BEFORE quantization/bit-depth reduction
- Limiter AFTER gain protects clipping
- Final safety limiter catches resampler/time-stretch intersample peaks
- Bit-perfect path disables ALL mutating stages except verified valid-bit-preserving container mapping

### 18.3 NaN/Inf/Denormal

- NaN → 0 by policy; ±Inf → clamp by policy
- Sanitization breaks bit-perfect; FloatSanitizationEvidence
- FTZ/DAZ only in float DSP path; integer bit-perfect unaffected

---

## §19. SPSC Strong Result Queue Plan (P6G-016)

### 19.1 Typed results (NOT bool, §15)

```cpp
enum class AudioSpscPushResult {
    Pushed, Full, Closed, GenerationMismatch, InvalidPacket
};

enum class AudioSpscPopResult {
    Popped, Empty, Closed, GenerationMismatch
};
```

### 19.2 Interface

```cpp
class AudioSpscRenderQueue {
public:
    AudioSpscPushResult try_push(AudioRenderPacket&& packet) noexcept;
    AudioSpscPopResult try_pop(AudioRenderPacket& out) noexcept;
    AudioSpscDepthSnapshot depth_snapshot() const noexcept;
    bool is_lock_free() const noexcept;
};
```

### 19.3 Production rule

- `is_lock_free()` must be true in production
- false allowed only in tests; must fail production architecture guard

### 19.4 Stale discard budget

- max_stale_discards_per_event = 64 packets (default)
- max_event_cpu_budget_ns = min(25% of device period, policy cap)
- If exceeded: stop discard loop + set async recovery flag + emit coalesced evidence

---

## §20. AudioRenderPacket Hot-path Memory Plan (P6G-016)

### 20.1 Static asserts (§10)

```cpp
static_assert(std::is_nothrow_move_constructible_v<AudioRenderPacket>);
static_assert(std::is_nothrow_destructible_v<AudioRenderPacket>);
```

### 20.2 Forbidden fields

- std::vector (hot-path allocation)
- std::string
- std::shared_ptr
- std::function
- owning heap block that frees on AudioDeviceThread
- backend COM pointer
- FFmpeg pointer
- Qt type
- borrowed span

### 20.3 Allowed fields

- pool slot handle
- owned lightweight render span descriptor
- small fixed metadata
- generation ids
- format hash
- remaining frame cursor
- claim state snapshot id

### 20.4 Partial consumption

- AudioDeviceThread advances packet cursor
- If packet remains → goes to AudioDeviceRemainderSlot
- Must NOT be pushed back into SPSC

---

## §21. Architecture Guard Scanner Mode Plan (P6G-037)

### 21.1 Four scan modes (§42)

| Mode | Scope | Comment handling |
|------|-------|-----------------|
| PublicHeadersStrict | `include/kivo/video/audio_plane/` | parser-based comment detection |
| ArchitectureRuleDocs | guard rule files | allowed as rule text |
| NegativeFixture | `tests/negative/` only | excluded from production include graph |
| BackendPrivate | `backend/src/video/audio_plane/` private roots | allowed under private roots only |

### 21.2 Forbidden token list (partial, §42)

```
windows.h audioclient.h mmdeviceapi.h ksmedia.h endpointvolume.h audiopolicy.h
AUDCLNT_REFERENCE_TIME GUID CoInitializeEx CoUninitialize
IMMNotificationClient PROPVARIANT PKEY
avcodec.h avformat.h swresample.h
AVFrame AVPacket AVCodecParameters AVCodecContext AVRational AVChannelLayout SwrContext
QString QByteArray QVector QSharedPointer QVariant QImage QWindow QObject QWidget
IAudioClient IAudioClient2 IAudioClient3 IAudioRenderClient IAudioClock
IMMDevice IAudioSessionControl ISimpleAudioVolume IAudioEndpointVolume
HRESULT WAVEFORMATEX WAVEFORMATEXTENSIBLE ComPtr wil::com_ptr
```

### 21.3 Path guard

- public include path must NOT contain `/wasapi/` `/ffmpeg/` `/qt/`
- public include graph must NOT depend on backend/src
- core target must NOT link wasapi backend
- wasapi backend may depend on contracts

### 21.4 Hot path guards

- allocation detector required
- noexcept static_assert checks required
- exception-disabled target preferred for realtime module
- operator new hook or allocator counter required in tests

---

## §22. Fuzz/Property/Benchmark/Audio-quality Test Plan (P6G-034)

### 22.1 Benchmark SLO defaults (§43)

| Metric | SLO |
|--------|-----|
| AudioDeviceThread event render p99 (normal release) | ≤ 1.0ms |
| AudioDeviceThread event render p99 (low-latency) | ≤ 0.5ms |
| Hot path allocation count | 0 |
| SPSC try_pop/try_push p99 | ≤ 10us |
| Seek-to-first-audio (normal) | ≤ 250ms |
| Same-format track switch | ≤ 150ms |
| Format-change track switch | ≤ 500ms |
| Device recovery default switch | ≤ 2000ms |
| Evidence hot submission p99 | ≤ 10us |

### 22.2 Audio quality golden tests

- bit-exact PCM null test
- 24-bit valid-bits preservation test
- S24 little/big-endian conversion test
- dither spectrum test
- resampler passband/stopband/THD+N test
- channel mapping impulse test
- downmix golden vector test
- gapless sample-boundary null test
- time-stretch latency/artifact baseline
- passthrough loopback/hash where possible

### 22.3 Concurrency stress

- seek while device lost
- track switch while format changed
- shutdown while decode init
- hotplug during GetBuffer
- P4 superseded command storm
- evidence queue pressure during recovery
- randomized command interleaving
- deterministic scheduler fake
- state-machine model test
- TSAN where possible

### 22.4 Benchmark environment (14 required fields)

CPU model, core count, Windows version, power mode, sample rate, channel count, sample format, buffer size, shared/exclusive mode, device period, backend fake/real, release/debug, sanitizer on/off, warmup duration + sample count + measurement overhead estimate

---

## §23. P6S Security/License/Claim Plan (P6G-030/051)

### 23.1 P6S scope (§45)

- codec claim policy
- Dolby / DTS / Atmos claim boundary
- DRM/private data redaction
- third-party notice boundary
- protected audio evidence policy
- crash dump redaction
- UI legal claim wording
- certification / license distinction

### 23.2 Must complete

- DetectedOnly vs Supported distinction
- BoundaryOnly mapping
- PassthroughActive vs LicensedForDistribution separation
- Certification claim requires CertifiedByVendor
- CodecPrivateDataBlob redaction
- Protected audio dump forbidden
- P8 claim text source locked to CapabilityRegistry
- Third-party license manifest shape
- Codec feature → license notice mapping
- Unsupported claim token blocked

### 23.3 Deferral allowed (but claim boundary CANNOT be deferred)

- Dolby production license enablement
- DTS production license enablement
- Atmos certification
- DTS:X certification
- Vendor certification badge

---

## §24. P6A Exact Allowed Files (P6G-053)

### 24.1 Production headers

- `include/kivo/video/audio_plane/` — 30 subdirectories, ~294 .hpp files (compile-only contract skeletons)

### 24.2 CMake

- `backend/CMakeLists.txt` — add `kivo_audio_plane_contracts` INTERFACE + `kivo_audio_plane_contract_tests` executable + `kivo_audio_plane_arch_guard` script + `kivo_audio_plane_negative_fixture` target

### 24.3 Tests

- `backend/tests/video/audio_plane/` — contract test .cpp files (mirroring 30 subdirectories)
- `backend/tests/video/audio_plane/negative/` — compile-fail negative fixtures

### 24.4 Architecture guard

- `backend/tools/governance/p6_audio_arch_guard.py` — Python scanner (4 modes)

### 24.5 Docs

- `docs/roadmap/p6_v2.0_execution_card.md` (this execution card, already created)
- `docs/roadmap/p6a_planning_draft_v2.md` (this document)
- `docs/roadmap/p6a_closure_report.md` (closure, P6G-043)

### 24.6 Foundational types

P6A must create minimal `kivo::Expected`, `kivo::Optional`, `kivo::Variant` in `include/kivo/video/audio_plane/foundation/` (or reuse if created elsewhere). These are compile-only wrappers.

---

## §25. P6A Forbidden Files (P6G-053)

### 25.1 Forbidden in P6A

- ANY `.cpp` production file (F-01: production .cpp forbidden, test .cpp allowed)
- ANY real WASAPI runtime (no IAudioClient/IAudioRenderClient/GetBuffer calls)
- ANY real FFmpeg decode (no AVFrame/AVPacket/avcodec calls)
- ANY real resampler (no SwrContext/libsamplerate calls)
- ANY real output playback (no device open/audio output)
- P6B/P6C/P6D code
- `backend/src/video/audio_plane/` production source (P6A = contracts only)
- `backend/include_private/` creation (private impl not P6A scope)
- Any modification to P5 `include/kivo/video_plane/` files
- Any modification to P4/P8 boundary types
- Any modification to existing `backend/include/kivo/cinema_engine/audio_core/` or `audio_output/` (legacy, replaced by P6 later)

### 25.2 Forbidden public header content

- WASAPI type names (IAudioClient, IAudioRenderClient, WAVEFORMATEX, etc.)
- FFmpeg type names (AVFrame, AVPacket, AVCodecContext, etc.)
- Qt type names (QString, QObject, etc.)
- Win32/COM type names (HRESULT, GUID, CoInitializeEx, etc.)
- `WasapiMode` in public enum
- `std::span` stored asynchronously or crossed threads

---

## §26. P6A Exit Criteria Table (§44, 26 items)

| # | Exit Criteria | Gate | Status |
|---|---------------|------|--------|
| 1 | C++23 toolchain lock | P6G-003/006 | PLANNED |
| 2 | public/private header surface locked | P6G-006 | PLANNED |
| 3 | P5 packet import contract | P6G-007/011 | PLANNED |
| 4 | data ownership split contract | P6G-012 | PLANNED |
| 5 | packet/frame pool contract | P6G-011/015 | PLANNED |
| 6 | decode lifecycle contract including TimedOut | P6G-013 | PLANNED |
| 7 | encoded passthrough parse contract with owned payload | P6G-014 | PLANNED |
| 8 | generation/epoch/wrap rule | P6G-017 | PLANNED |
| 9 | audio PTS checked rescale / wrap policy | P6G-018 | PLANNED |
| 10 | PCM current audible time formula | P6G-019 | PLANNED |
| 11 | PCM tail time formula | P6G-019 | PLANNED |
| 12 | passthrough audible anchor formula | P6G-014/019 | PLANNED |
| 13 | evidence topology | P6G-028 | PLANNED |
| 14 | claim freshness matrix | P6G-029 | PLANNED |
| 15 | bit-perfect strict gate | P6G-029 | PLANNED |
| 16 | gapless reserve P4/P6 command contract | P6G-022 | PLANNED |
| 17 | SPSC render queue strong result contract | P6G-016 | PLANNED |
| 18 | AudioRenderPacket hot-path memory contract | P6G-016 | PLANNED |
| 19 | WASAPI private boundary | P6G-016 (contract) | PLANNED |
| 20 | COM init boundary | P6G-016/033 | PLANNED |
| 21 | architecture guard runnable | P6G-037 | PLANNED |
| 22 | scanner mode exceptions defined | P6G-037 | PLANNED |
| 23 | contract tests planned | P6G-038 | PLANNED |
| 24 | fuzz/property/benchmark/audio-quality plan complete | P6G-034 | PLANNED |
| 25 | P6S security/claim boundary complete | P6G-030 | PLANNED |
| 26 | (all 30 subdirectories created) | P6G-006~036 | PLANNED |

**26/26 COVERED**

---

## §27. Natural Family Tree Summary (§6, 30 subdirectories, ~294 headers)

| # | Subdirectory | Headers | Key Types |
|---|-------------|---------|-----------|
| 1 | boundary/ | 12 | P5AudioPacketImportView, P4VideoTimelineProxySnapshot |
| 2 | command/ | 8 | AudioCommandId, AudioCommandIdempotencyPolicy |
| 3 | platform/ | 5 | AudioPlatformBackendId, AudioBackendRegistryLifecycle |
| 4 | output/ | 16 | AudioOutputHandle, AudioOutputMode, AudioOutputCapabilitySnapshot |
| 5 | packet/ | 9 | AudioPacketId, AudioPacketPoolContract |
| 6 | data/ | 9 | AudioSyncByteView, AudioOwnedBlockHandle, AudioPoolSlotHandle |
| 7 | decode/ | 9 | AudioDecodeBackendContract, DecodedAudioFrame |
| 8 | passthrough/ | 10 | EncodedAccessUnit, IEC61937PacketizationPolicy |
| 9 | frame/ | 6 | AudioFrameId, AudioFramePoolContract |
| 10 | render/ | 13 | AudioRenderPacket, AudioSpscQueueContract |
| 11 | generation/ | 8 | AudioEngineEpoch, AudioGenerationWrapPolicy |
| 12 | time/ | 11 | AudioPts, AudioCheckedRescale, AudioPtsWrapPolicy |
| 13 | clock/ | 18 | AudioClockCandidate, PcmCurrentAudibleTimeFormula |
| 14 | sync/ | 10 | AvSyncPolicy, MutedAudioClockPolicy |
| 15 | seek/ | 6 | AudioSeekModel, AudioSeekPrecision |
| 16 | gapless/ | 9 | AudioGaplessReserveContract, CodecDelayTrim |
| 17 | format/ | 16 | AudioCodecId, AudioFormatHash, AudioDrmBoundary |
| 18 | conversion/ | 8 | AudioDitherPolicy, AudioNoiseShapingPolicy |
| 19 | channel/ | 8 | AudioChannelLayout, AudioCanonicalSpeakerOrder |
| 20 | mix/ | 13 | DownmixPolicy, FinalSafetyLimiterPolicy, AudioDrcPolicy |
| 21 | rate/ | 8 | PlaybackRatePolicy, ScrubAudioPolicy |
| 22 | evidence/ | 16 | P6EvidenceRecord, AudioEvidenceTopology |
| 23 | capability/ | 11 | P6CapabilityRegistry, BitperfectStrictClaimGate |
| 24 | security/ | 11 | AudioLicenseBoundary, AudioPcmDumpPolicy |
| 25 | recovery/ | 7 | AudioFailureArbiter, AudioTerminalOutcome |
| 26 | advanced_format/ | 5 | DsdFormatBoundary (placeholder) |
| 27 | threading/ | 13 | AudioThreadModel, AudioRealtimePolicy |
| 28 | test_contract/ | 9 | AudioBenchmarkSlo, AudioQualityGoldenTestContract |
| 29 | terminology/ | 3 | AudioTermGlossary, AudioClaimTokenCanonicalName |
| 30 | version/ | 7 | P6BoundaryContractVersion, P6SchemaVersion |
| **Total** | | **~294** | |

---

## §28. CMake Target Structure

### 28.1 Four targets

| Target | Type | Links | Purpose |
|--------|------|-------|---------|
| `kivo_audio_plane_contracts` | INTERFACE | NOTHING | Public contract headers |
| `kivo_audio_plane_contract_tests` | EXECUTABLE | kivo_audio_plane_contracts + test framework | Contract tests |
| `kivo_audio_plane_arch_guard` | SCRIPT (Python) | N/A | Architecture guard scanner |
| `kivo_audio_plane_negative_fixture` | EXECUTABLE | kivo_audio_plane_contracts | Compile-fail tests |

### 28.2 C++ standard

```cmake
target_compile_features(kivo_audio_plane_contracts INTERFACE cxx_std_23)
```

### 28.3 Include directories

```cmake
target_include_directories(kivo_audio_plane_contracts INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)
```

### 28.4 Forbidden links

- WASAPI / FFmpeg / Qt / D3D11
- `kivo_video_plane_contracts` (P5) — P6 core must NOT depend on P5 contracts
- `kivo_cinema_engine` (legacy)

---

## §29. Risks / Blockers

| # | Risk | Mitigation |
|---|------|-----------|
| 1 | C++23 may not be fully supported by current MSVC | Verify MSVC 14.51+ supports required C++23 features; fallback to C++20 if blocked (requires user approval) |
| 2 | `kivo::Expected` / `Optional` / `Variant` don't exist | P6A creates minimal compile-only wrappers in `foundation/` |
| 3 | P5 `AudioPacketBoundaryItem` has only 5 fields, V2.0 needs 27 | P6 creates `P5AudioPacketImportView` as P6-side contract; P5 adapter fills it later |
| 4 | `AudioPipelineInputPlan` not found in repo | P6 creates it in `boundary/` as P6's view of P4 input plan |
| 5 | Legacy `cinema_engine/audio_core/` conflicts with P6 | P6 does NOT modify legacy; creates new `video/audio_plane/` namespace |

---

## §30. Ready for Implementation: YES ⚠️ CONDITIONAL

**Condition**: User must explicitly approve this Planning Draft before any header/CMake/test creation.

**After approval, P6A will create**:
- 30 subdirectories in `include/kivo/video/audio_plane/`
- ~294 compile-only contract .hpp files
- CMake targets (4)
- Contract test executable
- Architecture guard scanner (Python)
- Negative fixtures

**P6A will NOT create**:
- Any production .cpp
- Any real WASAPI/FFmpeg/resampler runtime
- Any P6B/P6C/P6D code
- Any modification to P5/P4/P8/legacy files

---

*END OF P6A PLANNING DRAFT V2*

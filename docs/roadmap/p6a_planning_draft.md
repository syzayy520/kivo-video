> **SUPERSEDED by p6a_planning_draft_v2.md (REV 002, based on V2.0 Design Lock Candidate)**
> **Superseded on**: 2026-06-28
> **Reason**: V1 was based on pre-V2.0 design (22 subdirectories, ~75 headers).
> V2.0 Design Lock Candidate requires 30 subdirectories, ~294 headers.
> V1 content preserved below for historical reference only. Do NOT use V1 for implementation.

# KIVO-VIDEO-P6A-PLANNING-DRAFT-001 (SUPERSEDED)

**Document Type**: planning-draft (design-only, ZERO implementation)
**Source**: P6 Audio Media Plane Design Lock Candidate V2.0 (USER CONFIRMED)
**Stage**: P6A (Planning Draft)
**Status**: DRAFT_FOR_REVIEW
**Generated**: 2026-06-28
**Rule**: IRON GATE F11/F12 — 禁止此 Draft APPROVED 前任何目录创建 / 头文件 / CMake / 实现

---

## IRON GATE COMPLIANCE

| # | Gate | Status |
|---|------|--------|
| 1 | 构思 | ✅ P6 Audio Media Plane Design Lock Candidate V2.0 (USER CONFIRMED) |
| 2 | 规划设计 | ⚠️ THIS DOCUMENT (draft for review) |
| 3 | 自然家族树 | ✅ Defined in §1 |
| 4 | 实现 | ⏳ BLOCKED (this is P6A only) |
| 5 | 单文件单职责 | ⏳ After implementation |
| 6 | 禁止平铺文件 | ⏳ After implementation |

---

## CHANGE LOG

| # | Version | Changes |
|---|---------|---------|
| 1 | 001 | Initial P6A Planning Draft — 26 scope items covered |

---

## 0. P6 SCOPE DEFINITION

### 0.1 一句话定义

P6 = **Audio Media Plane** — 完整的音频解码与输出平面：

- 从 P5 demux 接收 encoded audio packets
- 执行 decode (to PCM) **或** encoded passthrough
- 执行 resample/dither/quantize/limiter DSP
- 输出 PCM 到 WASAPI 设备
- 生成 AudioClockObservation 反馈给 P4
- 管理 packet pool / frame pool 内存
- 实现 bit-perfect / gapless / SPSC 队列 / evidence topology

### 0.2 P6 允许做

| # | Capability |
|---|-----------|
| 1 | 接收 P5 AudioPacketBoundaryItem |
| 2 | 管理 P5→P6 encoded packet import (data ownership split) |
| 3 | FFmpeg audio decode (avcodec_send_packet / avcodec_receive_frame) |
| 4 | Encoded passthrough parse path (S/PDIF 合规 header extraction) |
| 5 | Resample (libswresample or equivalent) |
| 6 | DSP chain: dither → quantize → limiter |
| 7 | PCM 写入 WASAPI 设备 (shared/exclusive) |
| 8 | Bit-perfect strict gate (no resample, no dither, no volume) |
| 9 | AudioPTS checked rescale / wrap |
| 10 | PCM current audible time formula |
| 11 | PCM tail time formula |
| 12 | Passthrough audible anchor |
| 13 | Generation/Epoch/Wrap plan |
| 14 | Packet pool + Frame pool memory management |
| 15 | SPSC strong result queue (decode → output) |
| 16 | AudioRenderPacket hot-path memory plan |
| 17 | Decode lifecycle (including TimedOut) |
| 18 | Gapless reserve P4/P6 command |
| 19 | Evidence topology: decode evidence, output evidence, clock drift evidence |
| 20 | Claim freshness matrix |
| 21 | Architecture guard scanner mode |
| 22 | Fuzz / property / benchmark / audio-quality test plan |
| 23 | P6S security / license / claim |

### 0.3 P6 禁止做

| # | Forbidden |
|---|----------|
| 1 | ❌ Demux (P5 owns demux) |
| 2 | ❌ Video decode/render (P5 owns video) |
| 3 | ❌ Subtitle rendering (P7 owns subtitle) |
| 4 | ❌ Provider selection (P3 owns provider) |
| 5 | ❌ Playback session control (P4 owns session) |
| 6 | ❌ UI (P8 owns UI) |
| 7 | ❌ Qt / QML |
| 8 | ❌ D3D11 / DXGI |

---

## 1. PROPOSED NATURAL FAMILY TREE

```
include/kivo/audio_plane/
│
├── identity/                        # P6 identity & epoch
│   ├── audio_pipeline_id.hpp        # AudioPipelineId (strong type)
│   ├── audio_generation.hpp         # P6 generation counter
│   ├── decode_epoch.hpp             # DecodeEpoch
│   ├── output_epoch.hpp             # OutputEpoch
│   └── audio_wrap_counter.hpp       # PTS wrap detection
│
├── packet/                          # P5→P6 packet import
│   ├── audio_packet_import.hpp      # Encoded packet from P5 demux
│   ├── packet_ownership.hpp         # Data ownership split (P5→P6 handoff)
│   ├── packet_pool.hpp              # Packet pool allocator
│   └── packet_sequence.hpp          # Monotonic packet sequence
│
├── decode/                          # FFmpeg audio decode
│   ├── decode_request.hpp           # Decode request (submit packet)
│   ├── decode_result.hpp            # Decode result (receive frame)
│   ├── decode_lifecycle.hpp         # Lifecycle: Idle→Decoding→Draining→TimedOut→Flushed→Closed
│   ├── decode_timeout_policy.hpp    # TimedOut threshold & behavior
│   └── decode_evidence.hpp          # Decode evidence (dropped, error, timeout)
│
├── passthrough/                     # Encoded passthrough (S/PDIF)
│   ├── passthrough_codec.hpp        # Passthrough codec enum (AC3/EAC3/DTS/TrueHD)
│   ├── passthrough_parse_result.hpp # Parsed S/PDIF header + owned payload
│   ├── passthrough_payload.hpp      # Owned passthrough byte buffer
│   └── passthrough_audible_anchor.hpp # Audible time anchor for passthrough
│
├── frame/                           # PCM frame pool
│   ├── pcm_frame.hpp                # Decoded PCM frame (AVFrame wrapper)
│   ├── frame_pool.hpp               # PCM frame pool allocator
│   ├── frame_format.hpp             # PcmFormat descriptor
│   └── frame_ownership.hpp          # Frame ownership transfer
│
├── resample/                        # Resample DSP
│   ├── resample_request.hpp         # Resample request contract
│   ├── resample_result.hpp          # Resample result
│   ├── resample_policy.hpp          # Resample policy (quality/speed)
│   └── resample_bypass_rule.hpp     # Bit-perfect bypass detection
│
├── dsp/                             # DSP chain
│   ├── dither_config.hpp            # Dither configuration
│   ├── quantize_config.hpp          # Quantize configuration
│   ├── limiter_config.hpp           # Limiter configuration
│   └── dsp_chain_order.hpp          # Dither→Quantize→Limiter order contract
│
├── timing/                          # Audio timing / PTS
│   ├── audio_pts_checked_rescale.hpp # Checked PTS rescale (overflow-safe)
│   ├── pts_wrap_detect.hpp          # PTS wrap/counter-overflow detection
│   ├── pcm_audible_time.hpp         # Current audible time formula (PCM)
│   ├── pcm_tail_time.hpp            # Tail time formula (remaining after stop)
│   └── passthrough_audible_time.hpp # Passthrough audible anchor
│
├── output/                          # WASAPI output
│   ├── output_device_contract.hpp   # Device open/close/start/stop contract
│   ├── output_buffer_contract.hpp   # Buffer write contract
│   ├── output_format_contract.hpp   # Device format negotiation
│   └── output_result.hpp            # Write result (samples, status)
│
├── clock/                           # Audio clock feedback
│   ├── audio_clock_observation.hpp  # Clock observation for P4
│   ├── clock_handover_policy.hpp    # P4→P6 clock handover
│   └── clock_drift_report.hpp       # Drift between P4 timeline & audio clock
│
├── bitperfect/                      # Bit-perfect gate
│   ├── bitperfect_eligibility.hpp   # Eligibility check
│   ├── bitperfect_gate.hpp          # Strict gate contract
│   ├── bitperfect_rejection.hpp     # Rejection reason
│   └── bitperfect_evidence.hpp      # Evidence: bit-perfect path active/inactive
│
├── gapless/                         # Gapless reserve
│   ├── gapless_reserve_command.hpp  # P4→P6 gapless reserve command
│   ├── gapless_tail_buffer.hpp      # Tail PCM buffer for crossfade
│   └── gapless_crossfade_policy.hpp # Crossfade policy
│
├── queue/                           # SPSC strong result queue
│   ├── spsc_decode_result.hpp       # SPSC queue item (decode→resample)
│   ├── spsc_output_packet.hpp       # SPSC queue item (resample→output)
│   ├── spsc_queue_contract.hpp      # SPSC capacity & backpressure
│   └── spsc_overflow_policy.hpp     # Overflow evidence
│
├── render_packet/                   # AudioRenderPacket hot path
│   ├── audio_render_packet.hpp      # Hot-path packet: PCM data + timestamp
│   ├── render_packet_memory.hpp     # Memory layout (inline + overflow)
│   └── render_packet_pool.hpp       # Render packet preallocated pool
│
├── evidence/                        # Evidence topology
│   ├── audio_decode_evidence.hpp    # Decode-level evidence
│   ├── audio_output_evidence.hpp    # Output-level evidence
│   ├── audio_clock_evidence.hpp     # Clock drift evidence
│   └── audio_evidence_topology.hpp  # Complete evidence topology contract
│
├── claim/                           # Claim freshness matrix
│   ├── claim_freshness_kind.hpp     # ClaimKind enum
│   ├── claim_freshness_record.hpp   # Claim record with staleness
│   └── claim_freshness_matrix.hpp   # Full freshness matrix contract
│
├── arch_guard/                      # Architecture guard
│   ├── p6_link_rules.hpp            # P6 target link rules
│   ├── p6_include_rules.hpp         # P6 include boundary rules
│   └── p6_scanner_mode.hpp          # Scanner mode configuration
│
├── test_plan/                       # Test plan contracts
│   ├── fuzz_target_spec.hpp         # Fuzz target specification
│   ├── property_test_spec.hpp       # Property test specification
│   ├── benchmark_spec.hpp           # Benchmark specification
│   └── audio_quality_spec.hpp       # Audio quality test spec (null test, THD)
│
├── security/                        # P6S security skeleton
│   ├── p6s_security_policy.hpp      # P6 security policy
│   ├── p6s_license_policy.hpp       # P6 license policy (FFmpeg/decoder)
│   └── p6s_claim_vocabulary.hpp     # P6 claim vocabulary
│
└── contract/                        # Top-level aggregation
    └── audio_media_plane_contract.hpp # P6 AudioMediaPlaneContract aggregation
```

**统计**: 22 个子目录, ~75 个头文件（P6A planning estimate）

---

## 2. REPO INVENTORY SUMMARY

### 2.1 Existing Repo Structure (pre-P6)

| Layer | Location | Files | Status |
|-------|----------|-------|--------|
| P0-C Contracts | `include/kivo/core/contract/` | 76 .hpp | ✅ IMPLEMENTED |
| P1 Backend Fakes | `backend/include/kivo/cinema_engine/` | 328 .hpp | ✅ IMPLEMENTED |
| P1 Backend Src | `backend/src/` | 177 .cpp | ✅ IMPLEMENTED |
| P2 Source Runtime | `backend/` (source_core, demux_core) | ~80 files | ✅ CLOSING |
| P3 Input Bridge | `docs/roadmap/p3_complete_line_v9` | planned | ⏳ READY |
| P4 Control Plane | `tmp_p4_readonly/` (DOCX) | design only | ⏳ PLANNED |
| P5 Video Plane | `include/kivo/video_plane/` | 219 .hpp | ✅ IMPLEMENTED |
| P5A Contracts | `include/kivo/video_plane/` (32 dirs) | 121 new | ✅ PUSHED |
| P6 Audio Plane | **THIS DOCUMENT** | 0 | ⏳ PLANNING |

### 2.2 Existing Audio-Related Files in backend/

| File | Purpose | P6 Impact |
|------|---------|-----------|
| `audio_core/audio_endpoint_contract.hpp` | AudioEndpointRuntimeContract | Will be **replaced** by P6 `output/output_device_contract.hpp` |
| `audio_core/audio_output_decision.hpp` | AudioOutputDecision | Will be absorbed into P6 `output/output_format_contract.hpp` |
| `audio_core/fake_audio_endpoint_service.hpp` | Fake endpoint | Will be replaced by P6 fake output device |
| `audio_output/wasapi_shared_pcm_writer.hpp` | WasapiSharedPcmWriter | Will be **replaced** by P6 `output/output_buffer_contract.hpp` |
| `audio_output/decoded_audio_frame_converter.hpp` | PCM converter | Will be absorbed into P6 `frame/` family |
| `audio_output/wasapi_internal.hpp` | WASAPI internal | Will be replaced by P6 adapter boundary |
| `decode_core/fake_decode_path.cpp` | Fake decode | Will be replaced by P6 real decode |
| `decode_core/fake_decode_runtime.cpp` | Fake decode runtime | Will be replaced by P6 real decode lifecycle |
| `third_party_adapter/ffmpeg_adapter/` | FFmpeg adapter | Will be **consumed** by P6 decode passthrough |

### 2.3 Existing Rust Audio Backend (kivo-audio-native-decode-pipeline-p0-009)

| Component | Status | Relevant to P6 |
|-----------|--------|---------------|
| DeviceBufferWriter | ✅ 304 tests | C++ equivalent = output/ family |
| RenderClientBoundary | ✅ contract only | C++ equivalent = output_buffer_contract |
| WasapiDeviceBufferWriter | ✅ runtime + tests | C++ equivalent = output/ + dsp/ |
| DeviceBufferWriter contract tests | ✅ 18 tests | Must replicate contract in C++ |
| SPSC queue | Not present | NEW for C++ |
| Frame pool | Not present | NEW for C++ |
| Decode lifecycle | Not present | NEW for C++ |

**Key insight**: Rust audio backend verified the WASAPI output path. P6 C++ covers decode + passthrough + DSP + pool + queue that Rust did NOT implement.

---

## 3. EXISTING P4/P5/P8 BOUNDARY INVENTORY

### 3.1 P4 → P6 Boundary

From P4 v1.8 Design Lock (tmp_p4_readonly_full.txt):

```cpp
struct AudioPipelineInputPlan {
    SessionId session_id;
    StreamRevision stream_revision;
    std::shared_ptr<IMediaStreamPipeline> encoded_stream;  // P4 encoded stream
    PlaybackTimelineState timeline_snapshot;
    PlaybackRateRatio playback_rate;
    AudioClockHandoverPolicy clock_handover_policy;
};
```

| Field | Type | P6 Must Consume |
|-------|------|----------------|
| `session_id` | P4::SessionId | ✅ P6 identity |
| `stream_revision` | P4::StreamRevision | ✅ P6 epoch tracking |
| `encoded_stream` | IMediaStreamPipeline* | ✅ P6 packet source |
| `timeline_snapshot` | P4::PlaybackTimelineState | ✅ P6 clock baseline |
| `playback_rate` | P4::PlaybackRateRatio | ✅ P6 rate for PTS calc |
| `clock_handover_policy` | P4::AudioClockHandoverPolicy | ✅ P6 clock authority |

**P4 contract**: P6 provides clock observations back to P4 via `DownstreamFeedbackRequest`.

### 3.2 P5 → P6 Boundary

From P5A output_boundary/:

```cpp
// audio_packet_boundary_item.hpp (GATE-023)
struct AudioPacketBoundaryItem {
    uint64_t packet_sequence{0};
    uint64_t media_timestamp{0};
    std::string audio_track_id;
    bool is_boundary{false};
};
```

| Field | P5 provides | P6 needs |
|-------|------------|---------|
| `packet_sequence` | ✅ P5 sequence | ✅ decode tracking |
| `media_timestamp` | ✅ uint64_t | ⚠️ P6 must convert to AudioPTS checked type |
| `audio_track_id` | ✅ string | ✅ track identity |
| `is_boundary` | ✅ bool | ✅ discontinuity handling |

**Gap**: P5 `AudioPacketBoundaryItem` does NOT carry:
- Encoded data pointer / size → P6 must define `AudioPacketImport` with data ownership
- Codec identification → P6 must receive from P5 demux separately  
- PTS timebase → P6 must receive from P5 demux metadata

### 3.3 P6 → P8 Boundary

P8 reads:
- `P5CapabilitySnapshot.user_visible_status` (from P5 capability registry)
- `P5CapabilitySnapshot.technical_detail_code`
- P6 audio capability display (audio device info, bit-perfect status, codec)

P6 must define:
- `AudioPlaneCapabilitySnapshot` → registered in P5 `P5CapabilityRegistry`
- Stable codes for P8-readable audio status

---

## 4. CMake TARGET STRUCTURE (PROPOSED)

```
INTERFACE kivo_audio_plane_contracts    ← P6A: all public contract headers
                                        ← LINK: NOTHING (pure contracts)
                                        ← NAMESPACE: kivo::audio_plane

STATIC   kivo_audio_plane_core          ← P6B+: C++ runtime (no platform deps)
                                        ← Responsible: decode, passthrough, resample, DSP,
                                        │   timing, bitperfect, gapless, evidence, claim
                                        ← LINK: kivo_audio_plane_contracts only
                                        ← MAY NOT LINK: WASAPI, Qt, D3D11
                                        ← MAY LINK: FFmpeg (libavcodec, optionally libswresample)

STATIC   kivo_audio_plane_ffmpeg_decode ← P6C: FFmpeg audio decode backend
                                        ← LINK: kivo_audio_plane_core + avcodec
                                        ← ISOLATED: src/audio_plane/ffmpeg_decode/ only

STATIC   kivo_audio_plane_wasapi_output ← P6D: WASAPI output backend
                                        ← LINK: kivo_audio_plane_core + mmdevapi
                                        ← ISOLATED: src/audio_plane/wasapi_output/ only
                                        ← Windows-only

EXE      kivo_audio_plane_contract_tests ← P6A+: all contract tests
                                        ← LINK: kivo_audio_plane_contracts only
```

**Link Rules** (parallel to P5 GATE-026):

| # | Rule | Check |
|---|------|-------|
| 1 | `kivo_audio_plane_contracts` links NOTHING | CMake dep graph |
| 2 | Contracts target has NO `target_link_libraries` to FFmpeg/WASAPI/Qt | CMake verify |
| 3 | `kivo_audio_plane_core` links contracts only, NO WASAPI | CMake + header scan |
| 4 | `kivo_audio_plane_ffmpeg_decode` MAY link avcodec, ONLY in `ffmpeg_decode/` dir | Path isolation |
| 5 | `kivo_audio_plane_wasapi_output` MAY link mmdevapi, ONLY in `wasapi_output/` dir | Path isolation |
| 6 | Public headers contain NO raw third-party types (AVFrame, AVCodecContext, IAudioClient) | Forbidden token scan |
| 7 | Binary exports contain NO forbidden symbols | `dumpbin /exports` scan |
| 8 | Core target exposes NO WASAPI/FFmpeg public ABI | Header + symbol scan |

---

## 5. P6A SCOPE: 26-POINT PLANNING INVENTORY

### 5.1 P5 Packet Import Contract Plan

**Problem**: P5 `AudioPacketBoundaryItem` does not carry encoded data.

**P6A plan**:
- Define `AudioPacketImport` struct in `packet/audio_packet_import.hpp`:
  - `AudioPacketBoundaryItem` (from P5)
  - `const uint8_t* encoded_data` (pointer to P5-owned or transferred buffer)
  - `size_t encoded_size`
  - `PacketOwnership ownership` (P5Owned / TransferredToP6)
- Define `PacketOwnership` enum in `packet/packet_ownership.hpp`:
  - `P5Owned` → P6 must copy data
  - `TransferredToP6` → P6 takes ownership, P5 must not touch

### 5.2 Data Ownership Split Plan

**Problem**: Encoded packets come from P5, decoded PCM goes to WASAPI. Who owns the memory?

**P6A plan**:
- `PacketOwnership` for encoded: P5Owned / TransferredToP6 (defined in packet/)
- `FrameOwnership` for decoded: P6Owned / LeasedToOutput (defined in frame/)
- `RenderPacketOwnership` for render: PreallocatedPool / BorrowedFromPool (defined in render_packet/)
- Rule: P6 **never** holds P5-owned packet after decode. P6 **always** returns borrowed frames to pool after WASAPI output completes.

### 5.3 Packet/Frame Pool Memory Plan

**Problem**: Hot-path decode → output must not allocate heap.

**P6A plan**:
- `PacketPool` (`packet/packet_pool.hpp`):
  - Fixed-size preallocated ring: N packets × max_packet_size
  - `acquire() → PacketLease`, `release(PacketLease)`
  - Overflow → evidence, backpressure on P5
- `FramePool` (`frame/frame_pool.hpp`):
  - Fixed-size preallocated ring: M frames × max_frame_samples × channels × sizeof(float)
  - `acquire() → FrameLease`, `release(FrameLease)`
  - Overflow → evidence, drop oldest

### 5.4 Decode Lifecycle Plan (including TimedOut)

**Problem**: FFmpeg decode can stall. Need lifecycle with timeout.

**P6A plan**:
- `DecodeLifecycle` enum in `decode/decode_lifecycle.hpp`:
  ```
  Idle → Decoding → Draining → Idle
  Idle → Decoding → TimedOut → Recovery → Idle
  Idle → Decoding → Flushed → Idle
  Any → Closed
  ```
- `DecodeTimeoutPolicy` in `decode/decode_timeout_policy.hpp`:
  - `decode_timeout_ms`: max time for `avcodec_receive_frame` before TimedOut
  - `recovery_action`: RetryOnce / FlushAndRetry / FailToEvidence
- `TimedOut` state: emit `AudioDecodeEvidence::DecodeTimedOut`, attempt recovery

### 5.5 Encoded Passthrough Parse Path Plan (with Owned Payload)

**Problem**: S/PDIF passthrough (AC3/EAC3/DTS/TrueHD) requires header parsing, not decode.

**P6A plan**:
- `PassthroughCodec` enum in `passthrough/passthrough_codec.hpp`: AC3, EAC3, DTS, DTSHD, TrueHD
- `PassthroughParseResult` in `passthrough/passthrough_parse_result.hpp`:
  - `PassthroughCodec codec`
  - `PassthroughPayload owned_payload` (byte-owning buffer, NOT raw pointer)
  - `uint64_t iec61937_data_type` (S/PDIF data type)
  - `bool parse_success`
- `PassthroughPayload` in `passthrough/passthrough_payload.hpp`:
  - `std::vector<uint8_t>` or `FixedArena<uint8_t, N>`
  - P6 **owns** this payload (copied from P5 or pool-allocated)
- Passthrough path: encoded packet → detect codec → parse header → build S/PDIF payload → send to WASAPI passthrough mode

### 5.6 Generation / Epoch / Wrap Plan

**Problem**: Multiple audio sequences need generation/epoch tracking.

**P6A plan**:
- `AudioGeneration` in `identity/audio_generation.hpp`: monotonic counter, increments on seek/recovery
- `DecodeEpoch` in `identity/decode_epoch.hpp`: increments on decode restart
- `OutputEpoch` in `identity/output_epoch.hpp`: increments on device reopen
- `AudioWrapCounter` in `identity/audio_wrap_counter.hpp`: detects PTS overflow/wrap

### 5.7 Audio PTS Checked Rescale / Wrap Plan

**Problem**: FFmpeg PTS can overflow, wrap, or have unknown timebase.

**P6A plan**:
- `AudioPtsCheckedRescale` in `timing/audio_pts_checked_rescale.hpp`:
  - `rescale(int64_t pts, AVRational src_tb, AVRational dst_tb) → CheckedResult<int64_t>`
  - Overflow → `CheckedResult::Overflow` evidence
  - Negative wrap → `CheckedResult::Wrapped` evidence
- `PtsWrapDetect` in `timing/pts_wrap_detect.hpp`:
  - Detects 33-bit counter wrap (common in media PTS)
  - `detect(int64_t prev, int64_t current) → WrapStatus`

### 5.8 PCM Current Audible Time Formula Plan

**Problem**: "Where are we hearing right now?" must be computable from PCM state.

**P6A plan**:
- `PcmAudibleTime` in `timing/pcm_audible_time.hpp`:
  ```
  AudibleTime = OutputDevice.SampleClock / OutputDevice.SampleRate
              - OutputDevice.BufferPendingFrames / OutputDevice.SampleRate
  ```
- Reference point: WASAPI `IAudioClock::GetPosition()` (adapter boundary)
- This is the **master clock candidate** for P4 timeline

### 5.9 PCM Tail Time Formula Plan

**Problem**: After stop command, how much PCM audio is still queued?

**P6A plan**:
- `PcmTailTime` in `timing/pcm_tail_time.hpp`:
  ```
  TailTime = (FramesInQueue + FramesInDeviceBuffer) / SampleRate
  ```
- This is used by:
  - P4 gapless handoff timing
  - P4 stop completion detection
  - Gapless reserve calculation

### 5.10 Passthrough Audible Anchor Plan

**Problem**: Passthrough has no sample clock. How to estimate audible position?

**P6A plan**:
- `PassthroughAudibleAnchor` in `passthrough/passthrough_audible_anchor.hpp`:
  - Uses: last submitted packet PTS + estimated device latency
  - `estimated_audible_pts_ms` = last_submitted_pts_ms - device_latency_ms_estimate
  - Lower confidence than PCM audible time (no sample-level precision)

### 5.11 Evidence Topology Plan

**Problem**: P6 needs structured evidence at 3 levels.

**P6A plan**:
- `AudioDecodeEvidence` in `evidence/`:
  - `DecodeError`, `DecodeTimedOut`, `PacketDropped`, `PassthroughParseFailed`
- `AudioOutputEvidence` in `evidence/`:
  - `DeviceLost`, `BufferUnderrun`, `BufferOverflow`, `FormatMismatch`
- `AudioClockEvidence` in `evidence/`:
  - `ClockDriftDetected`, `ClockSkew`, `ObservationStale`
- `AudioEvidenceTopology` in `evidence/`:
  - Aggregation of all 3 levels + mapping to `P5EvidenceRecord`

### 5.12 Claim Freshness Matrix Plan

**Problem**: Claims like "bit-perfect" must have freshness rules.

**P6A plan**:
- `ClaimFreshnessKind` enum in `claim/claim_freshness_kind.hpp`:
  - `BitPerfectActive`, `ResampleActive`, `DitherActive`, `PassthroughActive`
- `ClaimFreshnessRecord` in `claim/claim_freshness_record.hpp`:
  - Claim kind + timestamp + staleness_duration_ms
- `ClaimFreshnessMatrix` in `claim/claim_freshness_matrix.hpp`:
  - Maps claim → last verified → staleness threshold → auto-invalidates-if-exceeded

### 5.13 Bit-perfect Strict Gate Plan

**Problem**: Must detect and enforce bit-perfect path.

**P6A plan**:
- `BitperfectEligibility` in `bitperfect/bitperfect_eligibility.hpp`:
  - Check: same sample rate, same format, no volume, no DSP
  - `is_eligible(SrcFormat, DstFormat, DspConfig) → bool`
- `BitperfectGate` in `bitperfect/bitperfect_gate.hpp`:
  - Contract: if eligible and gate open → bypass resample/dither
  - if NOT eligible or gate closed → resample/dither path
- `BitperfectRejection` in `bitperfect/bitperfect_rejection.hpp`:
  - Reason enum: RateConvert, FormatMismatch, VolumeNonUnity, DSPActive
- `BitperfectEvidence` in `bitperfect/bitperfect_evidence.hpp`:
  - Emits evidence on path switch: Active→Bypassed or Bypassed→Active

### 5.14 Gapless Reserve P4/P6 Command Plan

**Problem**: Gapless track transitions need P4 to reserve P6 tail buffer.

**P6A plan**:
- `GaplessReserveCommand` in `gapless/gapless_reserve_command.hpp`:
  - `P4::SessionId next_session_id` (next track session)
  - `Milliseconds crossfade_duration_ms`
  - `bool reserve_tail_buffer`
  - P6 responds: `GaplessReserveResult { tail_available, tail_samples, tail_format }`
- `GaplessTailBuffer` in `gapless/gapless_tail_buffer.hpp`:
  - Holds last N samples before stop/flush
  - Accessed by next session for crossfade
- `GaplessCrossfadePolicy` in `gapless/gapless_crossfade_policy.hpp`:
  - Crossfade shape (linear, cosine), duration bounds

### 5.15 DSP Dither/Quantize/Limiter Order Plan

**Problem**: DSP chain must have fixed order.

**P6A plan**:
- `DspChainOrder` enum in `dsp/dsp_chain_order.hpp`:
  ```
  Resample → Dither → Quantize → Limiter → Output
  ```
- Each stage has config:
  - `DitherConfig`: type (TPDF/none), bit_depth_target
  - `QuantizeConfig`: target bit_depth, dither_on_quantize
  - `LimiterConfig`: ceiling_db, release_ms, lookahead_samples
- Bit-perfect path: bypass ALL stages

### 5.16 SPSC Strong Result Queue Plan

**Problem**: Decode thread → Output thread needs lock-free queue.

**P6A plan**:
- `SpscDecodeResult` in `queue/spsc_decode_result.hpp`:
  - Preallocated ring buffer for results from decode → resample
- `SpscOutputPacket` in `queue/spsc_output_packet.hpp`:
  - Preallocated ring buffer for results from resample → output
- `SpscQueueContract` in `queue/spsc_queue_contract.hpp`:
  - `capacity`: fixed at compile time or bootstrap
  - `push()`: returns WouldBlock if full
  - `pop()`: returns WouldBlock if empty
  - No mutex, no heap allocation
- `SpscOverflowPolicy` in `queue/spsc_overflow_policy.hpp`:
  - Overflow → emit AudioOutputEvidence::QueueOverflow
  - Consumer too slow → backpressure upstream

### 5.17 AudioRenderPacket Hot-Path Memory Plan

**Problem**: WASAPI output callback is realtime. Must not allocate.

**P6A plan**:
- `AudioRenderPacket` in `render_packet/audio_render_packet.hpp`:
  - `const float* pcm_data` (pointer to FramePool frame)
  - `AudioPts checked_pts`
  - `uint32_t frame_count`
  - `AudioRenderPacketFlags flags` (IsSilence, IsLast, IsPassthrough)
- `RenderPacketMemory` in `render_packet/render_packet_memory.hpp`:
  - Inline: up to 2 channels × 512 samples in struct
  - Overflow: FramePool pointer for larger buffers
- `RenderPacketPool` in `render_packet/render_packet_pool.hpp`:
  - Preallocated during bootstrap
  - Acquire from pool, release after output callback

### 5.18 Architecture Guard Scanner Mode Plan

**Problem**: Must prevent WASAPI/FFmpeg leaking into contracts.

**P6A plan**:
- `P6LinkRules` in `arch_guard/p6_link_rules.hpp`:
  - Replicates P5 GATE-026 pattern for P6 targets
- `P6IncludeRules` in `arch_guard/p6_include_rules.hpp`:
  - Forbidden: `#include <mmdeviceapi.h>`, `#include <audioclient.h>` in contracts
  - Forbidden: `#include <libavcodec/avcodec.h>` in contracts
- `P6ScannerMode` in `arch_guard/p6_scanner_mode.hpp`:
  - Scanner configuration: paths, rules, CI command

### 5.19 Fuzz/Property/Benchmark/Audio-Quality Test Plan

**Problem**: Audio needs specialized testing beyond unit tests.

**P6A plan**:
- `FuzzTargetSpec` in `test_plan/fuzz_target_spec.hpp`:
  - Fuzz: packet import, PTS rescale, decode lifecycle, passthrough parse
- `PropertyTestSpec` in `test_plan/property_test_spec.hpp`:
  - Property: decode(encode(samples)) ≈ samples, PTS monotonic, buffer underrun ≤ 0
- `BenchmarkSpec` in `test_plan/benchmark_spec.hpp`:
  - Benchmark: decode throughput, resample throughput, SPSC latency P99
- `AudioQualitySpec` in `test_plan/audio_quality_spec.hpp`:
  - Null test: bit-perfect path → output == input (byte-exact)
  - THD test: resample path → THD+N below threshold
  - Glitch test: seek + gapless → no audible glitch

### 5.20 P6S Security/License/Claim Plan

**Problem**: P6 contains FFmpeg decoders with license implications.

**P6A plan**:
- `P6sSecurityPolicy` in `security/p6s_security_policy.hpp`:
  - Decode sandbox boundary: FFmpeg decode in isolated context
  - Passthrough: no code execution on passthrough path
  - MP3 patent: decoder loaded lazily, can be disabled
- `P6sLicensePolicy` in `security/p6s_license_policy.hpp`:
  - FFmpeg LGPL/GPL compliance: dynamic linking, notice file
  - Dolby/DTS decoder license: passthrough only unless licensed
- `P6sClaimVocabulary` in `security/p6s_claim_vocabulary.hpp`:
  - Allowed claims: "AC3 passthrough", "PCM output", "24-bit bit-perfect"
  - Forbidden claims: "Dolby licensed decoder" (unless licensed)

### 5.21 Public/Private Header Surface Plan

**Public** (include/kivo/audio_plane/):
- All ~75 headers listed in §1
- Namespace: `kivo::audio_plane`
- Contract only in P6A

**Private** (backend/):
- `src/audio_plane/core/` → decode, resample, DSP, timing runtime
- `src/audio_plane/ffmpeg_decode/` → FFmpeg adapter
- `src/audio_plane/wasapi_output/` → WASAPI adapter
- `src/audio_plane/pool/` → PacketPool, FramePool, RenderPacketPool runtime
- `src/audio_plane/queue/` → SPSC queue runtime

**Tests**:
- `tests/audio_plane/contract/` → 22 test subdirectories mirroring include/
- `tests/audio_plane/integration/` → decode+resample, resample+output
- `tests/audio_plane/fuzz/` → fuzz targets
- `tests/audio_plane/benchmark/` → benchmarks
- `tests/audio_plane/audio_quality/` → null test, THD, glitch

---

## 6. P6A EXACT ALLOWED FILES

### 6.1 Allowed Directories (production)

| # | Directory | Purpose |
|---|-----------|---------|
| 1 | `include/kivo/audio_plane/` | Root contract directory |
| 2 | `include/kivo/audio_plane/identity/` | Identity, generation, epoch, wrap |
| 3 | `include/kivo/audio_plane/packet/` | P5→P6 packet import |
| 4 | `include/kivo/audio_plane/decode/` | Decode contract |
| 5 | `include/kivo/audio_plane/passthrough/` | Passthrough contract |
| 6 | `include/kivo/audio_plane/frame/` | PCM frame pool |
| 7 | `include/kivo/audio_plane/resample/` | Resample contract |
| 8 | `include/kivo/audio_plane/dsp/` | DSP chain contract |
| 9 | `include/kivo/audio_plane/timing/` | PTS, audible time, tail time |
| 10 | `include/kivo/audio_plane/output/` | WASAPI output contract |
| 11 | `include/kivo/audio_plane/clock/` | Clock feedback contract |
| 12 | `include/kivo/audio_plane/bitperfect/` | Bit-perfect gate |
| 13 | `include/kivo/audio_plane/gapless/` | Gapless reserve |
| 14 | `include/kivo/audio_plane/queue/` | SPSC queue contract |
| 15 | `include/kivo/audio_plane/render_packet/` | AudioRenderPacket |
| 16 | `include/kivo/audio_plane/evidence/` | Evidence topology |
| 17 | `include/kivo/audio_plane/claim/` | Claim freshness |
| 18 | `include/kivo/audio_plane/arch_guard/` | Architecture guard |
| 19 | `include/kivo/audio_plane/test_plan/` | Test plan contracts |
| 20 | `include/kivo/audio_plane/security/` | P6S security |
| 21 | `include/kivo/audio_plane/contract/` | Top-level aggregation |

**22 directories, ~75 headers**

### 6.2 Allowed Directories (tests)

| # | Directory | Purpose |
|---|-----------|---------|
| T1 | `tests/audio_plane/identity/` | Identity tests |
| T2 | `tests/audio_plane/packet/` | Packet import tests |
| T3 | `tests/audio_plane/decode/` | Decode contract tests |
| T4 | `tests/audio_plane/passthrough/` | Passthrough tests |
| T5 | `tests/audio_plane/frame/` | Frame pool tests |
| T6 | `tests/audio_plane/resample/` | Resample tests |
| T7 | `tests/audio_plane/dsp/` | DSP chain tests |
| T8 | `tests/audio_plane/timing/` | Timing tests |
| T9 | `tests/audio_plane/output/` | Output contract tests |
| T10 | `tests/audio_plane/clock/` | Clock tests |
| T11 | `tests/audio_plane/bitperfect/` | Bit-perfect tests |
| T12 | `tests/audio_plane/gapless/` | Gapless tests |
| T13 | `tests/audio_plane/queue/` | SPSC queue tests |
| T14 | `tests/audio_plane/render_packet/` | Render packet tests |
| T15 | `tests/audio_plane/evidence/` | Evidence tests |
| T16 | `tests/audio_plane/claim/` | Claim tests |
| T17 | `tests/audio_plane/arch_guard/` | Architecture guard tests |
| T18 | `tests/audio_plane/security/` | P6S tests |
| T19 | `tests/audio_plane/contract/` | Contract aggregation tests |

**19 test directories**

### 6.3 Allowed CMake Files

| # | File | Purpose |
|---|------|---------|
| 1 | `backend/CMakeLists.txt` | Add `kivo_audio_plane_contracts` target + test executable |

### 6.4 Allowed Docs

| # | File | Purpose |
|---|------|---------|
| 1 | `docs/roadmap/p6a_planning_draft.md` | THIS DOCUMENT |

---

## 7. P6A FORBIDDEN FILES

| # | Forbidden Item |
|---|---------------|
| F-01 | ❌ Production `.cpp` implementation files (NO `src/audio_plane/...`) |
| F-02 | ❌ `#include <windows.h>` in ANY P6A header |
| F-03 | ❌ `#include <mmdeviceapi.h>` or `<audioclient.h>` in ANY P6A header |
| F-04 | ❌ `#include <libavcodec/avcodec.h>` or `<libavformat/*>` in ANY P6A header |
| F-05 | ❌ `#include <libswresample/swresample.h>` in ANY P6A header |
| F-06 | ❌ Raw COM types (`IAudioClient`, `IAudioRenderClient`, etc.) in ANY P6A header |
| F-07 | ❌ Raw FFmpeg types (`AVCodecContext`, `AVFrame`, `AVPacket`) in ANY P6A header |
| F-08 | ❌ `src/audio_plane/` directory creation (no implementation in P6A) |
| F-09 | ❌ Any file outside the allowed directories listed in §6 |
| F-10 | ❌ WASAPI real runtime (WASAPI calls forbidden in P6A) |
| F-11 | ❌ FFmpeg real decode (FFmpeg calls forbidden in P6A) |
| F-12 | ❌ Real resampler (libswresample calls forbidden in P6A) |
| **Allow** | ✅ Test `.cpp` files ARE allowed for contract tests (`tests/audio_plane/`) |

---

## 8. P6A EXIT CRITERIA (26 ITEMS)

| # | Criterion | Coverage in Draft | Status |
|---|-----------|-------------------|--------|
| E1 | Repo inventory complete | §2: full backend/include/tests + Rust audit | ✅ COVERED |
| E2 | P4/P5/P8 boundary inventory | §3: P4 AudioPipelineInputPlan, P5 AudioPacketBoundaryItem, P8 capability | ✅ COVERED |
| E3 | Public/private header surface plan | §5.21: 22 include/ dirs, 4 src/ dirs, 19 test dirs | ✅ COVERED |
| E4 | P5 packet import contract plan | §5.1: AudioPacketImport + PacketOwnership | ✅ COVERED |
| E5 | Data ownership split plan | §5.2: PacketOwnership / FrameOwnership / RenderPacketOwnership | ✅ COVERED |
| E6 | Packet/frame pool memory plan | §5.3: PacketPool + FramePool contracts | ✅ COVERED |
| E7 | Decode lifecycle including TimedOut | §5.4: 7-state lifecycle + DecodeTimeoutPolicy | ✅ COVERED |
| E8 | Encoded passthrough parse path with owned payload | §5.5: PassthroughCodec + PassthroughParseResult + owned payload | ✅ COVERED |
| E9 | Generation/epoch/wrap plan | §5.6: AudioGeneration + DecodeEpoch + OutputEpoch + WrapCounter | ✅ COVERED |
| E10 | Audio PTS checked rescale/wrap plan | §5.7: CheckedRescale + PtsWrapDetect | ✅ COVERED |
| E11 | PCM current audible time formula | §5.8: AudibleTime = SampleClock/Rate − PendingFrames/Rate | ✅ COVERED |
| E12 | PCM tail time formula | §5.9: TailTime = (Queue + Device) / Rate | ✅ COVERED |
| E13 | Passthrough audible anchor | §5.10: estimated_audible_pts from last packet PTS + device latency | ✅ COVERED |
| E14 | Evidence topology | §5.11: Decode + Output + Clock evidence + topology aggregation | ✅ COVERED |
| E15 | Claim freshness matrix | §5.12: ClaimFreshnessKind + Record + Matrix | ✅ COVERED |
| E16 | Bit-perfect strict gate | §5.13: Eligibility + Gate + Rejection + Evidence | ✅ COVERED |
| E17 | Gapless reserve P4/P6 command | §5.14: GaplessReserveCommand + TailBuffer + CrossfadePolicy | ✅ COVERED |
| E18 | DSP dither/quantize/limiter order | §5.15: Resample→Dither→Quantize→Limiter→Output | ✅ COVERED |
| E19 | SPSC strong result queue | §5.16: SpscDecodeResult + SpscOutputPacket + QueueContract + Overflow | ✅ COVERED |
| E20 | AudioRenderPacket hot-path memory | §5.17: inline + overflow + pool | ✅ COVERED |
| E21 | Architecture guard scanner mode | §5.18: P6LinkRules + P6IncludeRules + ScannerMode | ✅ COVERED |
| E22 | Fuzz/property/benchmark/audio-quality test plan | §5.19: 4 test plan contracts | ✅ COVERED |
| E23 | P6S security/license/claim | §5.20: SecurityPolicy + LicensePolicy + ClaimVocabulary | ✅ COVERED |
| E24 | Exact allowed files | §6: 22 production + 19 test dirs + 2 CMake + 1 doc, ~75 headers | ✅ COVERED |
| E25 | Forbidden files | §7: F-01 through F-12, 12 hard stops | ✅ COVERED |
| E26 | Exit criteria table | §8: this table | ✅ COVERED |

**Coverage Summary**: 26/26 COVERED (0 NOT COVERED)

---

## 9. P6A PROPOSED FAMILY TREE SUMMARY

### 9.1 Subdirectory Count

| Category | Count |
|----------|-------|
| Production subdirectories | 22 |
| Estimated headers | ~75 |
| Test subdirectories | 19 |
| Estimated test files | ~75 |
| CMake files | 1 (backend/CMakeLists.txt) |

### 9.2 Namespace

```
namespace kivo::audio_plane::identity { ... }
namespace kivo::audio_plane::packet { ... }
namespace kivo::audio_plane::decode { ... }
namespace kivo::audio_plane::passthrough { ... }
namespace kivo::audio_plane::frame { ... }
namespace kivo::audio_plane::resample { ... }
namespace kivo::audio_plane::dsp { ... }
namespace kivo::audio_plane::timing { ... }
namespace kivo::audio_plane::output { ... }
namespace kivo::audio_plane::clock { ... }
namespace kivo::audio_plane::bitperfect { ... }
namespace kivo::audio_plane::gapless { ... }
namespace kivo::audio_plane::queue { ... }
namespace kivo::audio_plane::render_packet { ... }
namespace kivo::audio_plane::evidence { ... }
namespace kivo::audio_plane::claim { ... }
namespace kivo::audio_plane::arch_guard { ... }
namespace kivo::audio_plane::test_plan { ... }
namespace kivo::audio_plane::security { ... }
namespace kivo::audio_plane::contract { ... }
```

### 9.3 Dependency Direction

```
P4 Control Plane
  │
  ├─ AudioPipelineInputPlan ─→ kivo::audio_plane::clock/
  │                            kivo::audio_plane::output/
  │
P5 Video Plane
  │
  ├─ AudioPacketBoundaryItem ─→ kivo::audio_plane::packet/
  │
P6 Audio Plane (internal)
  │
  ├─ packet/ ──→ decode/ ──→ frame/ ──→ resample/ ──→ dsp/ ──→ render_packet/ ──→ output/
  │                  │                    │
  │                  └──→ passthrough/ ───┘ (bypass decode + resample + dsp)
  │
  ├─ timing/ (cross-cuts: consumed by output/, clock/, passthrough/)
  ├─ queue/  (cross-cuts: connects decode→resample, resample→output)
  ├─ bitperfect/ (cross-cuts: guards resample/ + dsp/)
  ├─ gapless/ (cross-cuts: consumed by output/)
  ├─ evidence/ (cross-cuts: emitted by all families)
  │
  └─ clock/ ──→ P4 Control Plane (feedback)
```

Ancestors consumed:
- P4 types (SessionId, StreamRevision, PlaybackTimelineState, PlaybackRateRatio)
- P5 types (AudioPacketBoundaryItem)

No new P0-C or P5 contracts needed. P6 is self-contained except for P4+P5 imports.

---

## 10. RISKS / BLOCKERS

| # | Risk | Severity | Mitigation |
|---|------|----------|-----------|
| R1 | P4 AudioPipelineInputPlan is not yet finalized (P4 still in design phase) | HIGH | P6A defines import contracts that can adapt to any P4 finalization. P6A does NOT depend on P4 implementation, only on P4 type definitions. These are already specified in P4 v1.8. |
| R2 | P5 AudioPacketBoundaryItem is too minimal (no codec, no timebase) | MEDIUM | P6A adds AudioPacketImport wrapper that extends P5's item. Codec/timebase metadata comes from P5 demux metadata, not P5 boundary item. P6A planning accounts for this. |
| R3 | Backend existing audio files (audio_core/, audio_output/) overlap with P6 target | LOW | These are fake/skeleton P1 files. P6 will replace them. P6A is contracts-only, no conflict with existing fake services. |
| R4 | Rust audio backend has no decode/passthrough/DSP/pool | NONE | Rust backend verified WASAPI output only. P6 C++ adds what Rust didn't do. |
| R5 | ~75 headers may exceed team capacity for single P6A sprint | MEDIUM | P6A is contracts-only (no runtime). 75 contract headers is comparable to P5A's 121 headers. Can be done in ~5 implementation rounds. |

---

## 11. WHETHER READY FOR P6A IMPLEMENTATION

| Check | Result |
|-------|--------|
| P6 Design Lock Candidate V2.0 confirmed? | ✅ USER CONFIRMED |
| All 26 planning items covered? | ✅ 26/26 COVERED |
| Repo inventory complete? | ✅ backend + include + Rust |
| P4/P5/P8 boundaries documented? | ✅ AudioPipelineInputPlan + AudioPacketBoundaryItem + P8 capability |
| Family tree is self-consistent? | ✅ 22 directories, dependency direction clear |
| No forbidden scope creep? | ✅ NO WASAPI, NO FFmpeg impl, NO P6B/P6C/P6D/P7/P8 |
| All forbidden files listed? | ✅ 12 hard stops |
| Exit criteria measurable? | ✅ 26 items, each mapped to document section |

### READY FOR P6A IMPLEMENTATION: YES ⚠️ CONDITIONAL

**Conditions**:
1. P4 AudioPipelineInputPlan type definitions must be **finalized** before P6A implementation begins. P6A headers will reference P4 types. But P6A Planning Draft itself does NOT depend on P4 being implemented — only that P4 type definitions are stable. P4 v1.8 Design Lock is sufficient.
2. User must confirm this Planning Draft before any header creation.

---

## 12. IMMEDIATE NEXT STEPS (AFTER APPROVAL)

1. Create directory scaffold (22 `include/kivo/audio_plane/*/` + 19 `tests/audio_plane/*/`)
2. Add CMake target: `kivo_audio_plane_contracts` INTERFACE + `kivo_audio_plane_contract_tests` EXECUTABLE
3. Wire into `backend/CMakeLists.txt`
4. Create headers in order: identity → packet → frame → timing → decode → passthrough → resample → dsp → bitperfect → gapless → queue → render_packet → output → clock → evidence → claim → arch_guard → test_plan → security → contract
5. Create test runners matching header structure
6. Run Gate Review: compile + CTest + 12 forbidden item checks

**Do NOT enter P6B/P6C/P6D. Do NOT enter P7/P8.**

---

**END OF P6A PLANNING DRAFT 001**
**Status**: DRAFT_FOR_REVIEW — awaiting user confirmation

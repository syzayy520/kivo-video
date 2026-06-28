# KIVO-VIDEO-P6B-PLANNING-DRAFT-001

**Document Type**: planning-draft (design-only, ZERO implementation)
**Source**: P6 Audio Media Plane Design Lock Candidate V2.0
**Predecessor**: P6A Contract Layer (CLOSED, commit 3d50ec3, 44/44 gates PASS)
**Stage**: P6B (Fake Runtime Bridge — platform-neutral, no real WASAPI/FFmpeg)
**Status**: DRAFT_FOR_REVIEW (REV 001 — 7-item revision applied)
**Generated**: 2026-06-28
**Revision History**:
- REV 000 (606e281): Initial draft
- REV 001 (this): 7-item revision per user feedback (workspace lock, dir count, platform-neutral arithmetic, honest SPSC, frozen contracts, guard coverage, fake adapter boundary)
**Rule**: IRON GATE — 禁止此 Draft APPROVED 前任何 .hpp / .cpp / CMake 创建

---

## WORKSPACE LOCK

| # | Rule |
|---|------|
| W1 | 唯一工作目录：`C:\kivo video` |
| W2 | 禁止新建 worktree |
| W3 | 禁止 clone 仓库到任何其他位置 |
| W4 | 禁止在 `C:\kivo video` 之外的任何目录读写 P6B 相关文件 |
| W5 | 所有 commit/push 必须在 `C:\kivo video` 本仓库 `kivo-video-p5a-contract-layer-001` 分支完成 |
| W6 | 禁止外部目录作为 "临时" / "备份" / "参考" |

**Violation handling**: 任何违反 WORKSPACE LOCK 的操作立即 STOP，不继续执行。

---

## IRON GATE COMPLIANCE

| # | Gate | Status |
|---|------|--------|
| 1 | 构思 | ✅ V2.0 Design Lock Candidate (USER CONFIRMED) |
| 2 | 规划设计 | ⚠️ THIS DOCUMENT (draft for review) |
| 3 | 自然家族树 | ✅ Defined in §1 |
| 4 | 实现 | ⏳ BLOCKED (awaiting user approval) |
| 5 | 单文件单职责 | ⏳ After implementation |
| 6 | 禁止平铺文件 | ⏳ After implementation |

---

## 0. P6B SCOPE DEFINITION

### 0.1 一句话定义

P6B = **P5 Audio Packet Import Bridge + Core Fake Runtime** — 平台无关的 fake runtime 层，验证 P6A 合同的可实现性，不涉及真实 WASAPI/FFmpeg。

### 0.2 P6B 允许做

| # | Capability |
|---|-----------|
| 1 | Fake P5→P6 audio packet import runtime（validation + ownership transfer + sequence check） |
| 2 | Fake audio data ownership runtime（SyncByteView / OwnedBlock / PoolSlot 的 fake 实现） |
| 3 | Fake packet pool runtime（preallocated ring allocator, cap enforcement） |
| 4 | Fake frame pool runtime（preallocated ring allocator, recycle, pressure behavior） |
| 5 | Fake generation/epoch runtime（monotonic counter + wrap detection） |
| 6 | Fake audio PTS checked rescale runtime（platform-neutral fake checked arithmetic, overflow detection） |
| 7 | Fake evidence record runtime（基础 evidence 收集 + ID 生成） |
| 8 | Fake decode backend lifecycle runtime（16-state machine, TimedOut closure） |
| 9 | Fake SPSC render queue runtime（deterministic fake, strong typed result, NOT production lock-free） |
| 10 | Fake render packet runtime（noexcept move/destruct, pool-backed） |

### 0.3 P6B 禁止做

| # | Forbidden |
|---|----------|
| 1 | ❌ Real WASAPI calls（IAudioClient / IAudioRenderClient / GetBuffer） |
| 2 | ❌ Real FFmpeg decode（avcodec_send_packet / avcodec_receive_frame） |
| 3 | ❌ Real resampler（libswresample / libsamplerate） |
| 4 | ❌ Real audio output playback（device open / actual sound） |
| 5 | ❌ Real passthrough parse（S/PDIF header extraction from real stream） |
| 6 | ❌ P6C code（FFmpeg decode backend implementation） |
| 7 | ❌ P6D code（WASAPI output backend implementation） |
| 8 | ❌ P6E+ code（resampler / DSP chain / clock / sync runtime） |
| 9 | ❌ Qt / QML |
| 10 | ❌ D3D11 / DXGI |
| 11 | ❌ P4/P5/P8 file modification |
| 12 | ❌ Public contract header modification（P6A 298 contract headers are FROZEN — see §2.1） |

### 0.4 P6B vs P6A vs P6C/D Boundary

```
P6A (CLOSED):  Contract headers only (298 .hpp, compile-only, no runtime)
P6B (THIS):    Fake runtime (header-only, platform-neutral, no real backends)
P6C (FUTURE):  FFmpeg decode backend (real avcodec, isolated in decode/ffmpeg/)
P6D (FUTURE):  WASAPI output backend (real IAudioClient, isolated in output/wasapi/)
P6E+ (FUTURE): Resampler / DSP / clock / sync / gapless runtime
```

### 0.5 P6B Fake Adapter Boundary (CRITICAL)

P6B contains fake adapters for decode, render, and passthrough. These are **NOT** P6C/P6D/P6E runtime:

| P6B Fake Adapter | What it IS | What it is NOT |
|------------------|-----------|----------------|
| `runtime/decode/fake_decode_backend.hpp` | Fake 16-state lifecycle + deterministic frame generation (no real codec) | NOT P6C (P6C = real FFmpeg avcodec in `decode/ffmpeg/`) |
| `runtime/decode/fake_decode_lifecycle_controller.hpp` | Fake transition validation (no real backend) | NOT P6C lifecycle controller |
| `runtime/render/fake_spsc_render_queue.hpp` | Deterministic fake ring (NOT lock-free, `is_lock_free()=false`) | NOT P6E (P6E = real lock-free SPSC) |
| `runtime/render/fake_render_packet_pool.hpp` | Fake preallocated pool (no real device thread) | NOT P6D render runtime |
| `runtime/render/fake_device_remainder_slot.hpp` | Fake single-slot (no real AudioDeviceThread) | NOT P6D device runtime |
| `runtime/passthrough/fake_passthrough_parser.hpp` | Fake deterministic payload (no real S/PDIF header extraction) | NOT P6E passthrough runtime |

**Rule**: P6B fake adapters simulate state machines and data flow ONLY. They do NOT call any real backend API. Real FFmpeg decode (P6C), real WASAPI output (P6D), real resampler/DSP (P6E) are explicitly out of scope.

---

## 1. PROPOSED NATURAL FAMILY TREE

### 1.1 Directory Structure

P6B fake runtime headers go into `include/kivo/video/audio_plane/` under new `runtime/` subdirectories. These are **fake implementations** (header-only, no .cpp), not contracts.

```
include/kivo/video/audio_plane/
│
├── runtime/                         # P6B fake runtime root (NEW)
│   │
│   ├── import_bridge/               # P5→P6 packet import runtime
│   │   ├── fake_p5_packet_importer.hpp       # FakeP5PacketImporter (validation, ownership transfer)
│   │   ├── packet_import_validator.hpp        # PacketImportValidator (stream/track/gen/epoch/sequence checks)
│   │   ├── packet_ownership_transfer.hpp      # PacketOwnershipTransfer (P5Owned → P6Owned state machine)
│   │   └── packet_sequence_checker.hpp        # PacketSequenceChecker (monotonicity, duplicate, disorder)
│   │
│   ├── data/                        # Audio data ownership runtime
│   │   ├── fake_data_handle_manager.hpp       # FakeDataHandleManager (3 variants lifecycle)
│   │   ├── owned_block_allocator.hpp          # OwnedBlockAllocator (heap-backed, noexcept release)
│   │   └── sync_byte_view_guard.hpp           # SyncByteViewGuard (RAII, sync-call-only enforcement)
│   │
│   ├── pool/                        # Packet + frame pool runtime
│   │   ├── fake_packet_pool.hpp               # FakePacketPool (preallocated ring, cap enforcement)
│   │   ├── fake_frame_pool.hpp                # FakeFramePool (preallocated ring, recycle, pressure)
│   │   └── pool_pressure_policy.hpp           # PoolPressurePolicy (Normal/Live/Bitperfect/Passthrough)
│   │
│   ├── generation/                  # Generation/epoch runtime
│   │   ├── fake_generation_counter.hpp        # FakeGenerationCounter (monotonic, wrap detection)
│   │   └── fake_epoch_manager.hpp             # FakeEpochManager (EngineEpoch reset, staleness check)
│   │
│   ├── time/                        # PTS checked rescale runtime
│   │   ├── fake_checked_rescale.hpp           # FakeCheckedRescale (platform-neutral checked multiply, overflow detection)
│   │   └── fake_pts_wrap_detector.hpp         # FakePtsWrapDetector (33-bit wrap, backward jump)
│   │
│   ├── evidence/                    # Evidence record runtime
│   │   ├── fake_evidence_collector.hpp        # FakeEvidenceCollector (record, ID gen, TTL)
│   │   └── fake_evidence_id_generator.hpp     # FakeEvidenceIdGenerator (atomic fetch_add, wrap)
│   │
│   ├── decode/                      # Fake decode backend lifecycle
│   │   ├── fake_decode_backend.hpp            # FakeDecodeBackend (16-state machine, TimedOut)
│   │   └── fake_decode_lifecycle_controller.hpp # FakeDecodeLifecycleController (transitions, cancellation)
│   │
│   ├── render/                      # Fake SPSC queue + render packet
│   │   ├── fake_spsc_render_queue.hpp         # FakeSpscRenderQueue (deterministic fake, NOT production lock-free, strong result)
│   │   ├── fake_render_packet_pool.hpp        # FakeRenderPacketPool (preallocated, acquire/release)
│   │   └── fake_device_remainder_slot.hpp     # FakeDeviceRemainderSlot (single slot, overwrite detection)
│   │
│   └── passthrough/                 # Fake passthrough parse backend
│       └── fake_passthrough_parser.hpp        # FakePassthroughParser (owned payload, no real S/PDIF)
```

### 1.2 Test Directory Structure

```
backend/tests/video/audio_plane/
│
├── p6a_contract_tests.cpp           # P6A (EXISTING, FROZEN)
├── negative/                        # P6A negative fixtures (EXISTING, FROZEN)
│
└── p6b_runtime_tests/               # P6B fake runtime tests (NEW)
    ├── import_bridge_tests.cpp      # B1-B4: importer, validator, ownership, sequence
    ├── data_handle_tests.cpp        # B5-B7: handle manager, owned block, sync view guard
    ├── pool_tests.cpp               # B8-B10: packet pool, frame pool, pressure policy
    ├── generation_tests.cpp         # B11-B12: generation counter, epoch manager
    ├── time_tests.cpp               # B13-B14: checked rescale, PTS wrap detector
    ├── evidence_tests.cpp           # B15-B16: evidence collector, ID generator
    ├── decode_tests.cpp             # B17-B18: decode backend, lifecycle controller
    ├── render_tests.cpp             # B19-B21: SPSC queue, render packet pool, remainder slot
    └── passthrough_tests.cpp        # B22: fake passthrough parser
```

### 1.3 Statistics

| Category | Count |
|----------|-------|
| Runtime directories | 10 (1 root `runtime/` + 9 subdirs: import_bridge, data, pool, generation, time, evidence, decode, render, passthrough) |
| Runtime headers | ~24 |
| Test files | 9 |
| CMake targets | 1 new (kivo_audio_plane_runtime_tests) |
| Namespace | kivo::video::audio_plane::runtime::* |

---

## 2. REPO INVENTORY (P6B-specific)

### 2.1 P6A Contract Layer (FROZEN — 298 headers, consumed by P6B, NOT modifiable)

**FROZEN RULE**: P6A delivered 298 `.hpp` contract headers across 30 design subdirectories + 1 `foundation/` directory under `include/kivo/video/audio_plane/`. These headers are **FROZEN**:

- P6B MUST NOT modify, delete, or rename any P6A contract header.
- P6B MUST NOT add headers to any P6A contract subdirectory (boundary/, command/, platform/, output/, packet/, data/, decode/, passthrough/, frame/, render/, generation/, time/, clock/, sync/, seek/, gapless/, format/, conversion/, channel/, mix/, rate/, evidence/, capability/, security/, recovery/, advanced_format/, threading/, test_contract/, terminology/, version/, foundation/).
- P6B runtime headers MUST be added ONLY under `include/kivo/video/audio_plane/runtime/` (new root).
- If P6B discovers missing contract fields, STOP and request P6A contract amendment — do NOT patch in `runtime/`.

| P6A Directory | Headers | P6B Consumes |
|-----------|---------|--------------|
| `boundary/` | 12 | P5AudioPacketImportView (27 fields), P5AudioPacketImportResult (12 values) |
| `packet/` | 9 | AudioPacketImportView, AudioPacketImportResult, AudioPacketPoolContract |
| `data/` | 9 | AudioDataOwnershipMode (6 variants), AudioOwnedBlockHandle, AudioPoolSlotHandle, AudioSyncByteView |
| `decode/` | 9 | AudioDecodeBackendState (16 states), AudioDecodeBackendLifecycle |
| `frame/` | 6 | AudioFramePoolContract, AudioFrameBuffer |
| `render/` | 13 | AudioRenderPacket, AudioSpscResult, AudioSpscQueueContract, AudioDeviceRemainderSlot |
| `generation/` | 8 | AudioEngineEpoch, AudioPipelineGeneration, AudioGenerationWrapPolicy |
| `time/` | 11 | AudioCheckedRescale, AudioPtsWrapPolicy, TimestampOverflow |
| `evidence/` | 16 | P6EvidenceRecord, AudioEvidenceIdGenerator |
| `passthrough/` | 10 | EncodedAccessUnit, EncodedAccessUnitOwnedPayload |
| `foundation/` | 4 | ContractMetadata, kivo::Expected, kivo::Optional, kivo::Variant |

### 2.2 P5B Reference Pattern (consumed as design template)

P5B implemented 8 header-only fake runtime files in `include/kivo/video_plane/input_bridge/`:
- `p4_video_input_bridge.hpp` — FakeP4VideoInputBridge (state machine, lease window)
- `demux_buffer_pool.hpp` — DemuxBufferPool (allocation cap, stats)
- `encoded_input_io_adapter.hpp` — FakeEncodedInputIOAdapter (open/read/seek/probe)
- `out_of_band_backpressure.hpp` — OutOfBandBackpressure (atomic publish)
- `attachment_safety_descriptor.hpp` — AttachmentSafetyDescriptor (MIME, quota)
- `fake_demux_backend.hpp` — FakeDemuxBackend (known corpus, keyframe detection)
- `progress_watchdog.hpp` — ProgressWatchdog (stall detection)
- `ffmpeg_copy_to_pool_contract.hpp` — FFmpegCopyToPoolContract (validate, zero, reject)

**Key pattern**: Header-only (no .cpp), fake state machine, deterministic data, no real I/O.

### 2.3 CMake Target Structure (P6B addition)

```cmake
# P6B: Fake Runtime Tests (links P6A contracts, no real backends)
add_executable(kivo_audio_plane_runtime_tests
  tests/video/audio_plane/p6b_runtime_tests/import_bridge_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/data_handle_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/pool_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/generation_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/time_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/evidence_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/decode_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/render_tests.cpp
  tests/video/audio_plane/p6b_runtime_tests/passthrough_tests.cpp
)
target_link_libraries(kivo_audio_plane_runtime_tests PRIVATE kivo_audio_plane_contracts)
target_compile_features(kivo_audio_plane_runtime_tests PRIVATE cxx_std_23)
```

**Link Rules**:
- `kivo_audio_plane_runtime_tests` links ONLY `kivo_audio_plane_contracts`
- NO link to WASAPI / FFmpeg / Qt / D3D11
- NO link to `kivo_cinema_engine` (legacy)
- NO link to `kivo_video_plane_contracts` (P5)

### 2.4 Architecture Guard Coverage (P6B runtime headers MUST be guarded)

The existing P6A architecture guard (`backend/tools/governance/p6_audio_arch_guard.py`) MUST be extended to cover P6B runtime headers. The guard has 4 modes (§42):

| Mode | P6B Coverage Requirement |
|------|--------------------------|
| `PublicHeadersStrict` | MUST scan `include/kivo/video/audio_plane/runtime/**/*.hpp` — same forbidden token list as P6A contracts (no WASAPI/FFmpeg/Qt/Win32/COM types) |
| `ArchitectureRuleDocs` | Unchanged (rule docs only) |
| `NegativeFixture` | Unchanged (P6A negative fixtures) |
| `BackendPrivate` | P6B does NOT create `backend/src/video/audio_plane/` — this mode remains scanning for forbidden private backend roots |

**Guard extension plan (P6B implementation phase, NOT this draft)**:
- `PublicHeadersStrict` mode scan path expanded from `include/kivo/video/audio_plane/*.hpp` (P6A contracts only) to also include `include/kivo/video/audio_plane/runtime/**/*.hpp` (P6B fake runtime).
- P6B runtime headers MUST pass the same forbidden token scan as P6A contracts.
- P6B runtime headers MUST NOT contain real WASAPI/FFmpeg/Qt/Win32/COM types (they are fake, platform-neutral).
- CTest registration: `p6_audio_arch_guard_public_headers_strict` test (#181) MUST continue to PASS after P6B headers are added.

**Forbidden in P6B runtime headers** (same as P6A contracts):
- `IAudioClient`, `IAudioRenderClient`, `IMMDevice`, `HRESULT`, `WAVEFORMATEX`
- `AVFrame`, `AVPacket`, `AVCodecContext`, `SwrContext`
- `QString`, `QByteArray`, `QObject`
- `#include <windows.h>`, `#include <audioclient.h>`, `#include <avcodec.h>`

---

## 3. P6B SCOPE: 22-POINT PLANNING INVENTORY

### 3.1 Fake P5 Packet Importer (B1)

**Problem**: P6A defines `P5AudioPacketImportView` (27 fields) and `AudioPacketImportResult` (12 values), but no runtime import logic.

**P6B plan**:
- `FakeP5PacketImporter` in `runtime/import_bridge/fake_p5_packet_importer.hpp`:
  - `import(view: P5AudioPacketImportView, ownership: AudioDataOwnershipMode) → Expected<AudioPacketImportResult, ImportError>`
  - State machine: NotInitialized → Ready → Importing → Draining → Closed
  - Deterministic packet generation (no real P5 demux)
  - Ownership mode handling: SyncBorrowedByteView → copy; MoveOwnedBlock → take; PoolSlotOwnedByP6 → allocate slot

### 3.2 Packet Import Validator (B2)

**Problem**: P6A `P5AudioPacketImportContract` defines 6 validation rules but no runtime.

**P6B plan**:
- `PacketImportValidator` in `runtime/import_bridge/packet_import_validator.hpp`:
  - `validate_stream_id_track_id_generation_epoch(view) → bool`
  - `validate_packet_sequence_monotonicity(view, last_sequence) → bool`
  - `validate_codec_private_data_before_decode_init(view) → bool`
  - `never_store_p5_raw_pointer()` — static_assert enforced
  - `never_expose_packet_bytes_to_p8()` — static_assert enforced
  - `protected_encrypted_summary_redacted(view) → bool`

### 3.3 Packet Ownership Transfer (B3)

**Problem**: P6A defines 6 ownership modes but no transfer state machine.

**P6B plan**:
- `PacketOwnershipTransfer` in `runtime/import_bridge/packet_ownership_transfer.hpp`:
  - State machine: P5Owned → Transferring → P6Owned → Released
  - Rejected before transfer: original owner releases
  - Rejected after move accepted: P6 releases or retires into pool
  - DeferredBackpressure: ownership remains with sender
  - SupersededByFlushOrSeek: P6 retires by generation discard

### 3.4 Packet Sequence Checker (B4)

**Problem**: P6A §7 defines packet disorder policy but no runtime.

**P6B plan**:
- `PacketSequenceChecker` in `runtime/import_bridge/packet_sequence_checker.hpp`:
  - `check(view, last_state) → SequenceCheckResult`
  - Non-monotonic with discontinuity: accept discontinuity path
  - Non-monotonic without discontinuity: reject + emit PacketOrderViolation
  - Duplicate packet: reject unless explicit retry marker
  - Backward timestamp with monotonic sequence: timestamp discontinuity + evidence

### 3.5 Fake Data Handle Manager (B5)

**Problem**: P6A defines 3 handle variants but no unified manager.

**P6B plan**:
- `FakeDataHandleManager` in `runtime/data/fake_data_handle_manager.hpp`:
  - Manages SyncByteView / OwnedBlock / PoolSlot lifecycle
  - `acquire_sync_view(packet_id) → AudioSyncByteView`
  - `acquire_owned_block(packet_id, size) → AudioOwnedBlockHandle`
  - `acquire_pool_slot(packet_id) → AudioPoolSlotHandle`
  - `release(handle_id) → ReleaseResult`
  - Span lifetime enforcement: sync view cannot outlive call

### 3.6 Owned Block Allocator (B6)

**Problem**: P6A `AudioOwnedBlockHandle` requires noexcept release_fn but no allocator.

**P6B plan**:
- `OwnedBlockAllocator` in `runtime/data/owned_block_allocator.hpp`:
  - Heap-backed allocation (fake, not pool)
  - `allocate(size) → AudioOwnedBlockHandle`
  - release_fn is noexcept (sets flag, no heavy free on device thread)
  - Heavy release deferred to non-realtime thread

### 3.7 Sync Byte View Guard (B7)

**Problem**: P6A §8.1 mandates span is sync-call-only, must not be stored/cross-thread.

**P6B plan**:
- `SyncByteViewGuard` in `runtime/data/sync_byte_view_guard.hpp`:
  - RAII wrapper that invalidates view on scope exit
  - `acquire() → AudioSyncByteView` (valid only in current scope)
  - `release()` — invalidates, prevents storage
  - Debug-mode assertion: guard must be released before thread exit

### 3.8 Fake Packet Pool (B8)

**Problem**: P6A `AudioPacketPoolContract` defines cap enforcement but no allocator.

**P6B plan**:
- `FakePacketPool` in `runtime/pool/fake_packet_pool.hpp`:
  - Preallocated ring: N packets × max_packet_size
  - `acquire() → Expected<PoolSlot, PoolFull>`
  - `release(slot)` — returns slot to ring
  - Cap enforcement: max_bytes, max_packets, max_duration_ms
  - Hot-path allocation forbidden: all slots preallocated at init
  - Overflow → evidence, backpressure

### 3.9 Fake Frame Pool (B9)

**Problem**: P6A `AudioFramePoolContract` requires noexcept move/destruct but no pool.

**P6B plan**:
- `FakeFramePool` in `runtime/pool/fake_frame_pool.hpp`:
  - Preallocated ring: M frames × max_frame_samples × channels × sizeof(float)
  - `acquire() → Expected<FrameLease, PoolFull>`
  - `release(lease)` — returns frame to pool
  - noexcept move/destruct verified via static_assert
  - No heap free on device thread (release = mark slot free, no delete)

### 3.10 Pool Pressure Policy (B10)

**Problem**: P6A §9 defines 4 pressure behaviors but no runtime.

**P6B plan**:
- `PoolPressurePolicy` in `runtime/pool/pool_pressure_policy.hpp`:
  - Normal playback: backpressure upstream
  - Live: drop according to live policy
  - Bit-perfect: rebuffer/fail rather than silent drop
  - Passthrough: drop only complete stale encoded access unit

### 3.11 Fake Generation Counter (B11)

**Problem**: P6A §13 defines generation/epoch/wrap rule but no runtime.

**P6B plan**:
- `FakeGenerationCounter` in `runtime/generation/fake_generation_counter.hpp`:
  - `increment() → Expected<Generation, WrapDetected>`
  - Wrap detection: increment would produce 0 or wrap → EngineEpochResetRequired
  - `is_fresh(snapshot_epoch, snapshot_gen, current_epoch, current_gen) → bool`
  - Cross-epoch comparison forbidden (only equality of full pair)

### 3.12 Fake Epoch Manager (B12)

**Problem**: P6A §13 defines EngineEpoch reset but no runtime.

**P6B plan**:
- `FakeEpochManager` in `runtime/generation/fake_epoch_manager.hpp`:
  - `reset_epoch() → NewEngineEpoch`
  - On reset: stop active pipelines, invalidate snapshots, reset generation counters
  - `current_epoch() → AudioEngineEpoch`
  - `staleness_check(snapshot) → StalenessResult`

### 3.13 Fake Checked Rescale (B13)

**Problem**: P6A `AudioCheckedRescale` defines overflow-safe policy but no runtime.

**P6B plan**:
- `FakeCheckedRescale` in `runtime/time/fake_checked_rescale.hpp`:
  - `rescale(pts: int64_t, src_tb: AudioTimeBase, dst_tb: AudioTimeBase) → Expected<int64_t, TimestampOverflow>`
  - Platform-neutral fake checked arithmetic: detect overflow BEFORE multiply via pre-check (e.g., `a > INT64_MAX / b`), NO platform-specific intrinsics (`_mul128`, `__int128`)
  - Overflow → TimestampOverflow::MultiplyOverflow, fail safely
  - No long-term float accumulation

### 3.14 Fake PTS Wrap Detector (B14)

**Problem**: P6A `AudioPtsWrapPolicy` defines 33-bit wrap but no runtime.

**P6B plan**:
- `FakePtsWrapDetector` in `runtime/time/fake_pts_wrap_detector.hpp`:
  - `detect(prev: int64_t, current: int64_t) → WrapStatus`
  - 33-bit counter wrap detection
  - Backward jump detection (negative delta beyond tolerance)
  - Seek/flush/discontinuity resets reorder/drift windows

### 3.15 Fake Evidence Collector (B15)

**Problem**: P6A `P6EvidenceRecord` defines record structure but no collector.

**P6B plan**:
- `FakeEvidenceCollector` in `runtime/evidence/fake_evidence_collector.hpp`:
  - `record(kind, severity, correlation_id) → EvidenceId`
  - TTL enforcement: expired evidence auto-dropped
  - ChainRoot evidence: cannot be silently dropped (compact critical counter)
  - Storage: in-memory ring (fake, no disk)

### 3.16 Fake Evidence ID Generator (B16)

**Problem**: P6A §32 defines ID generation but no runtime.

**P6B plan**:
- `FakeEvidenceIdGenerator` in `runtime/evidence/fake_evidence_id_generator.hpp`:
  - `next_id() → EvidenceId` (atomic fetch_add relaxed)
  - 0 reserved invalid
  - Wrap triggers EngineEpochResetRequired for local counters
  - No UUID generation on hot path

### 3.17 Fake Decode Backend (B17)

**Problem**: P6A §11 defines 16-state lifecycle including TimedOut but no runtime.

**P6B plan**:
- `FakeDecodeBackend` in `runtime/decode/fake_decode_backend.hpp`:
  - State machine: NotCreated → Created → WaitingCodecPrivateData → Opening → Ready → Decoding → Draining/Flushing/FormatChanging → TimedOut → Resetting/FailedFatal → Closed/Superseded
  - `submit_packet(packet) → Expected<DecodeAccept, DecodeReject>`
  - `receive_frame() → Expected<DecodedFrame, DecodeStatus>` (WouldBlock / TimedOut / Eos)
  - TimedOut state: deadline exceeded, no frames published after deadline
  - Fake decode: deterministic frame generation (no real FFmpeg)
  - **This is a FAKE ADAPTER, NOT P6C**: No `avcodec_send_packet` / `avcodec_receive_frame` / `AVFrame` / `AVCodecContext`. Real FFmpeg decode is P6C scope (future, isolated in `decode/ffmpeg/`).

### 3.18 Fake Decode Lifecycle Controller (B18)

**Problem**: P6A §11 defines transitions but no controller.

**P6B plan**:
- `FakeDecodeLifecycleController` in `runtime/decode/fake_decode_lifecycle_controller.hpp`:
  - `transition(target_state) → Expected<void, TransitionError>`
  - Cancellation: seek/flush/track-switch/close cancels init/decode
  - Superseded: newer generation, must not publish frames
  - Timeout thresholds: normal=3000ms, live=5000ms, protected=policy-defined

### 3.19 Fake SPSC Render Queue (B19)

**Problem**: P6A §15 defines strong-typed SPSC contract but no runtime.

**P6B plan**:
- `FakeSpscRenderQueue` in `runtime/render/fake_spsc_render_queue.hpp`:
  - Deterministic fake ring buffer (preallocated, single-threaded test use)
  - **NOT production lock-free**: P6B fake does NOT claim `is_lock_free() == true`. Real lock-free SPSC deferred to P6E.
  - **This is a FAKE ADAPTER, NOT P6E**: No real lock-free atomics / memory-order-optimized producer-consumer. Real lock-free SPSC is P6E scope (future).
  - `try_push(packet) → AudioSpscPushResult` (Pushed/Full/Closed/GenMismatch/InvalidPacket)
  - `try_pop(out) → AudioSpscPopResult` (Popped/Empty/Closed/GenMismatch)
  - `is_lock_free() → bool` returns `false` in fake impl (honest; production guard expects `true` only in P6E)
  - Memory order: producer release, consumer acquire (documented contract; fake uses seq_cst for simplicity)
  - Stale discard budget: max 64/event, max_event_cpu_budget_ns = min(25% device_period, cap)

### 3.20 Fake Render Packet Pool (B20)

**Problem**: P6A §10 requires noexcept move/destruct but no pool.

**P6B plan**:
- `FakeRenderPacketPool` in `runtime/render/fake_render_packet_pool.hpp`:
  - Preallocated during bootstrap
  - `acquire() → Expected<RenderPacketLease, PoolFull>`
  - `release(lease)` — returns to pool
  - static_assert: `is_nothrow_move_constructible_v<AudioRenderPacket>`
  - static_assert: `is_nothrow_destructible_v<AudioRenderPacket>`

### 3.21 Fake Device Remainder Slot (B21)

**Problem**: P6A §16 defines single-slot remainder but no runtime.

**P6B plan**:
- `FakeDeviceRemainderSlot` in `runtime/render/fake_device_remainder_slot.hpp`:
  - Single slot owned by AudioDeviceThread only
  - `set(packet) → Expected<void, OverwriteViolation>`
  - `consume() → Optional<AudioRenderPacket>`
  - Overwrite: set atomic recovery flag, increment compact critical counter, return from event loop

### 3.22 Fake Passthrough Parser (B22)

**Problem**: P6A §12 defines EncodedAccessUnit with owned payload but no parser.

**P6B plan**:
- `FakePassthroughParser` in `runtime/passthrough/fake_passthrough_parser.hpp`:
  - `parse(packet) → Expected<EncodedAccessUnit, ParseError>`
  - Owned payload only (no borrowed span, verified by negative fixture P6G-039)
  - Fake: deterministic payload generation (no real S/PDIF header extraction)
  - Codec policy: AC3/EAC3/DTS/TrueHD mapping (fake, no real codec detection)
  - **This is a FAKE ADAPTER, NOT P6E**: No real IEC61937 burst framing / S/PDIF header parsing. Real passthrough parse runtime is P6E scope (future).

---

## 4. P6B EXACT ALLOWED FILES

### 4.1 Allowed Directories (production — fake runtime headers)

| # | Directory | Purpose |
|---|-----------|---------|
| 1 | `include/kivo/video/audio_plane/runtime/` | P6B fake runtime root |
| 2 | `include/kivo/video/audio_plane/runtime/import_bridge/` | P5→P6 import fake runtime |
| 3 | `include/kivo/video/audio_plane/runtime/data/` | Data ownership fake runtime |
| 4 | `include/kivo/video/audio_plane/runtime/pool/` | Packet + frame pool fake runtime |
| 5 | `include/kivo/video/audio_plane/runtime/generation/` | Generation/epoch fake runtime |
| 6 | `include/kivo/video/audio_plane/runtime/time/` | PTS checked rescale fake runtime |
| 7 | `include/kivo/video/audio_plane/runtime/evidence/` | Evidence collector fake runtime |
| 8 | `include/kivo/video/audio_plane/runtime/decode/` | Decode backend fake runtime |
| 9 | `include/kivo/video/audio_plane/runtime/render/` | SPSC queue + render packet fake runtime |
| 10 | `include/kivo/video/audio_plane/runtime/passthrough/` | Passthrough parser fake runtime |

**10 directories, ~24 headers**

### 4.2 Allowed Directories (tests)

| # | Directory | Purpose |
|---|-----------|---------|
| T1 | `backend/tests/video/audio_plane/p6b_runtime_tests/` | P6B fake runtime tests |

**1 test directory, 9 test files**

### 4.3 Allowed CMake Files

| # | File | Purpose |
|---|------|---------|
| 1 | `backend/CMakeLists.txt` | Add `kivo_audio_plane_runtime_tests` target |

### 4.4 Allowed Docs

| # | File | Purpose |
|---|------|---------|
| 1 | `docs/roadmap/p6b_planning_draft.md` | THIS DOCUMENT |

---

## 5. P6B FORBIDDEN FILES

| # | Forbidden Item |
|---|---------------|
| F-01 | ❌ Any modification to P6A contract headers (`include/kivo/video/audio_plane/{boundary,command,platform,output,packet,data,decode,passthrough,frame,render,generation,time,clock,sync,seek,gapless,format,conversion,channel,mix,rate,evidence,capability,security,recovery,advanced_format,threading,test_contract,terminology,version}/`) |
| F-02 | ❌ Any real WASAPI call (IAudioClient / IAudioRenderClient / GetBuffer) |
| F-03 | ❌ Any real FFmpeg call (avcodec_send_packet / avcodec_receive_frame) |
| F-04 | ❌ Any real resampler (SwrContext / libsamplerate) |
| F-05 | ❌ Any real audio output playback (device open) |
| F-06 | ❌ Any production .cpp file (P6B = header-only fake runtime) |
| F-07 | ❌ P6C code (FFmpeg decode backend implementation) |
| F-08 | ❌ P6D code (WASAPI output backend implementation) |
| F-09 | ❌ P6E+ code (resampler / DSP / clock / sync runtime) |
| F-10 | ❌ Any file outside allowed directories |
| F-11 | ❌ P4/P5/P8 file modification |
| F-12 | ❌ Qt / QML / D3D11 / DXGI |
| **Allow** | ✅ Test .cpp files ARE allowed (`backend/tests/video/audio_plane/p6b_runtime_tests/`) |
| **Allow** | ✅ Fake runtime .hpp headers ARE allowed (`include/kivo/video/audio_plane/runtime/`) |

---

## 6. P6B EXIT CRITERIA (22 ITEMS)

| # | Criterion | Coverage | Status |
|---|-----------|----------|--------|
| E1 | Fake P5 packet importer runtime | §3.1 (B1) | ✅ DESIGNED |
| E2 | Packet import validator runtime | §3.2 (B2) | ✅ DESIGNED |
| E3 | Packet ownership transfer runtime | §3.3 (B3) | ✅ DESIGNED |
| E4 | Packet sequence checker runtime | §3.4 (B4) | ✅ DESIGNED |
| E5 | Fake data handle manager runtime | §3.5 (B5) | ✅ DESIGNED |
| E6 | Owned block allocator runtime | §3.6 (B6) | ✅ DESIGNED |
| E7 | Sync byte view guard runtime | §3.7 (B7) | ✅ DESIGNED |
| E8 | Fake packet pool runtime | §3.8 (B8) | ✅ DESIGNED |
| E9 | Fake frame pool runtime | §3.9 (B9) | ✅ DESIGNED |
| E10 | Pool pressure policy runtime | §3.10 (B10) | ✅ DESIGNED |
| E11 | Fake generation counter runtime | §3.11 (B11) | ✅ DESIGNED |
| E12 | Fake epoch manager runtime | §3.12 (B12) | ✅ DESIGNED |
| E13 | Fake checked rescale runtime | §3.13 (B13) | ✅ DESIGNED |
| E14 | Fake PTS wrap detector runtime | §3.14 (B14) | ✅ DESIGNED |
| E15 | Fake evidence collector runtime | §3.15 (B15) | ✅ DESIGNED |
| E16 | Fake evidence ID generator runtime | §3.16 (B16) | ✅ DESIGNED |
| E17 | Fake decode backend (16-state + TimedOut) | §3.17 (B17) | ✅ DESIGNED |
| E18 | Fake decode lifecycle controller | §3.18 (B18) | ✅ DESIGNED |
| E19 | Fake SPSC render queue (strong typed) | §3.19 (B19) | ✅ DESIGNED |
| E20 | Fake render packet pool (noexcept) | §3.20 (B20) | ✅ DESIGNED |
| E21 | Fake device remainder slot (overwrite detection) | §3.21 (B21) | ✅ DESIGNED |
| E22 | Fake passthrough parser (owned payload) | §3.22 (B22) | ✅ DESIGNED |

**Coverage: 22/22 DESIGNED (0 NOT COVERED)**

---

## 7. RISKS / BLOCKERS

| # | Risk | Severity | Mitigation |
|---|------|----------|-----------|
| R1 | P6A contract types may be incomplete for runtime use | MEDIUM | P6B may discover missing fields. If so, STOP and request P6A contract amendment before proceeding. Do NOT modify P6A headers directly. |
| R2 | Fake SPSC queue is NOT production lock-free | LOW | P6B fake `is_lock_free()` returns `false` (honest). Real lock-free SPSC deferred to P6E. Architecture guard must NOT be weakened to accept `false` in production. |
| R3 | Platform-neutral checked arithmetic may have edge cases | LOW | P6B uses pre-check overflow detection (divide-before-multiply). This is a fake; real high-performance checked rescale deferred to P6E. Edge cases covered by unit tests. |
| R4 | 24 fake runtime headers may exceed single sprint capacity | LOW | Header-only fake runtime (no .cpp). Comparable to P5B's 8 headers. Can be done in ~3 implementation rounds. |
| R5 | Fake decode backend 16-state machine is complex | MEDIUM | Start with minimal state transitions (NotCreated→Ready→Decoding→Closed). Add TimedOut/Draining/Flushing incrementally. |

---

## 8. WHETHER READY FOR P6B IMPLEMENTATION

| Check | Result |
|-------|--------|
| P6A contract layer closed? | ✅ 44/44 gates PASS, commit 3d50ec3 |
| V2.0 design lock confirmed? | ✅ USER CONFIRMED |
| All 22 planning items covered? | ✅ 22/22 DESIGNED |
| P6A contracts consumed (not modified)? | ✅ FROZEN |
| No forbidden scope creep? | ✅ NO real WASAPI/FFmpeg/resampler |
| All forbidden files listed? | ✅ 12 hard stops |
| Exit criteria measurable? | ✅ 22 items, each mapped to B1-B22 |
| P5B pattern available as template? | ✅ 8 header-only fake runtime files |

### READY FOR P6B IMPLEMENTATION: YES ⚠️ CONDITIONAL

**Conditions**:
1. User must explicitly approve this Planning Draft before any header creation.
2. P6A contract headers must remain FROZEN — P6B consumes them, does not modify them.
3. If P6B discovers missing contract fields, STOP and request P6A amendment.

---

## 9. IMMEDIATE NEXT STEPS (AFTER APPROVAL)

1. Create directory scaffold (10 `include/kivo/video/audio_plane/runtime/*/` + 1 test dir)
2. Add CMake target: `kivo_audio_plane_runtime_tests` executable
3. Wire into `backend/CMakeLists.txt`
4. Create fake runtime headers in order:
   import_bridge → data → pool → generation → time → evidence → decode → render → passthrough
5. Create test runners (9 files, B1-B22)
6. Run Gate Review: compile + CTest + forbidden token scan + architecture guard (guard MUST cover P6B runtime headers — see §2.4)

**Do NOT enter P6C/P6D. Do NOT enter P7/P8.**

---

**END OF P6B PLANNING DRAFT 001**
**Status**: DRAFT_FOR_REVIEW — awaiting user confirmation

# KIVO-VIDEO-P6C-PLANNING-DRAFT-001

**Document Type**: planning-draft (design-only, ZERO implementation)
**Source**: P6 Audio Media Plane Design Lock Candidate V2.0 (§2, §4.1, §5, §11, §12, §33)
**Predecessors**: P6A Contract Layer (CLOSED), P6B Fake Runtime Bridge (CLOSED)
**Stage**: P6C (FFmpeg Audio Decode Backend — real avcodec, isolated)
**Status**: DRAFT_FOR_REVIEW
**Generated**: 2026-06-28
**Rule**: IRON GATE — 禁止此 Draft APPROVED 前任何 .hpp / .cpp / CMake 创建

---

## WORKSPACE LOCK

| # | Rule |
|---|------|
| W1 | 唯一工作目录：`C:\kivo video` |
| W2 | 禁止新建 worktree |
| W3 | 禁止 clone 仓库到任何其他位置 |
| W4 | 禁止在 `C:\kivo video` 之外的任何目录读写 P6C 相关文件 |
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

## 0. P6C SCOPE DEFINITION

### 0.1 一句话定义

P6C = **FFmpeg Audio Decode Backend** — 真实的 FFmpeg avcodec 音频解码后端，隔离在 `decode/ffmpeg/` 私有目录中，消费 P6A decode 合同 + P6B fake runtime 接口模式，不涉及 WASAPI 输出或 resampler。

### 0.2 P6C 允许做

| # | Capability |
|---|-----------|
| 1 | Real FFmpeg avcodec 音频解码（`avcodec_send_packet` / `avcodec_receive_frame`） |
| 2 | AVCodecContext 创建、配置、销毁（隔离在 backend private） |
| 3 | AVFrame → P6 DecodedAudioFrame 转换（包含 planar→interleaved 如需要） |
| 4 | Codec private data 传递给 avcodec（从 P6A AudioCodecPrivateDataRef） |
| 5 | 16-state lifecycle 真实实现（含 TimedOut 超时检测） |
| 6 | Flush（`avcodec_flush_buffers`）+ Drain（send NULL packet + receive until Eos） |
| 7 | FormatChanged 检测（avcodec 参数变化触发） |
| 8 | Decode evidence 真实生成（InitOk / InitFailed / DecodeOk / DecodeFailed / TimedOut / Flushed / Drained / FormatChanged） |
| 9 | Decode timeout 监控（normal=3000ms, live=5000ms, protected=policy） |
| 10 | FFmpeg adapter 边界隔离（private headers 不暴露到 public） |

### 0.3 P6C 禁止做

| # | Forbidden |
|---|----------|
| 1 | ❌ Real WASAPI calls（P6D scope） |
| 2 | ❌ Real resampler（libswresample，P6E scope） |
| 3 | ❌ Real audio output playback（P6D scope） |
| 4 | ❌ Real passthrough parse（S/PDIF / IEC61937，P6E scope） |
| 5 | ❌ Real DSP chain（dither / quantize / limiter，P6E scope） |
| 6 | ❌ Real clock / sync / A/V offset runtime（P6E scope） |
| 7 | ❌ P6D code（WASAPI output backend） |
| 8 | ❌ P6E+ code（resampler / DSP / clock / sync runtime） |
| 9 | ❌ Qt / QML |
| 10 | ❌ D3D11 / DXGI |
| 11 | ❌ P4/P5/P8 file modification |
| 12 | ❌ P6A contract header modification（298 headers FROZEN） |
| 13 | ❌ P6B runtime header modification（22 headers FROZEN） |
| 14 | ❌ Public header 中暴露 FFmpeg 类型（AVFrame/AVPacket/AVCodecContext/AVCodecParameters/SwrContext） |
| 15 | ❌ Public include path 包含 `/wasapi/` `/ffmpeg/` `/qt/` |

### 0.4 P6C Boundary (CRITICAL)

| Aspect | What it IS | What it is NOT |
|--------|-----------|----------------|
| P6C | Real FFmpeg avcodec decode backend | NOT P6D (WASAPI output) |
| P6C | Isolated in `backend/src/video/audio_plane/decode/ffmpeg/` | NOT public include |
| P6C | Consumes P6A decode contracts | NOT modifies P6A contracts |
| P6C | Follows P6B fake decode interface pattern | NOT modifies P6B runtime |
| P6C | Real `avcodec_send_packet` / `avcodec_receive_frame` | NOT resampler / output / DSP |
| P6C | AVFrame→DecodedAudioFrame conversion | NOT AVFrame exposure to public |
| P6C | FFmpeg LGPL/GPL compliance | NOT Dolby/DTS licensed decode |

---

## 1. PROPOSED NATURAL FAMILY TREE

### 1.1 Directory Structure

P6C creates **private backend** files only. No public headers. All FFmpeg types are isolated in `backend/src/video/audio_plane/decode/ffmpeg/` and `backend/include_private/video/audio_plane/decode/ffmpeg/`.

```
backend/
│
├── src/video/audio_plane/decode/ffmpeg/          # P6C FFmpeg decode backend (NEW)
│   │
│   ├── internal/                                  # FFmpeg internal headers
│   │   ├── ffmpeg_codec_context_guard.hpp          # RAII guard for AVCodecContext*
│   │   ├── ffmpeg_frame_guard.hpp                  # RAII guard for AVFrame*
│   │   ├── ffmpeg_packet_guard.hpp                 # RAII guard for AVPacket*
│   │   ├── ffmpeg_codec_registry.hpp               # AudioCodecId → AVCodecID mapping
│   │   └── ffmpeg_decode_config.hpp                # FFmpeg decode configuration
│   │
│   ├── ffmpeg_decode_backend.cpp                   # FFmpegDecodeBackend implementation
│   ├── ffmpeg_decode_lifecycle.cpp                 # Lifecycle controller (real transitions)
│   ├── ffmpeg_frame_converter.cpp                  # AVFrame → DecodedAudioFrame conversion
│   ├── ffmpeg_codec_private_data.cpp               # CodecPrivateData → avcodec extradata
│   └── ffmpeg_decode_evidence_emitter.cpp          # Decode evidence generation
│
├── include_private/video/audio_plane/decode/ffmpeg/  # P6C private interface (NEW)
│   │
│   ├── ffmpeg_decode_backend.hpp                   # FFmpegDecodeBackend class declaration
│   ├── ffmpeg_decode_lifecycle.hpp                  # FFmpegDecodeLifecycleController class
│   ├── ffmpeg_frame_converter.hpp                   # AVFrame→DecodedAudioFrame converter
│   ├── ffmpeg_codec_private_data.hpp                # CodecPrivateData adapter
│   └── ffmpeg_decode_evidence_emitter.hpp           # Evidence emitter interface
│
└── tests/video/audio_plane/p6c_ffmpeg_decode_tests/  # P6C tests (NEW)
    ├── ffmpeg_decode_backend_tests.cpp               # C1-C8: backend lifecycle + decode
    ├── ffmpeg_frame_converter_tests.cpp               # C9-C12: AVFrame conversion
    └── ffmpeg_decode_evidence_tests.cpp               # C13-C15: evidence emission
```

### 1.2 Statistics

| Category | Count |
|----------|-------|
| Backend source directories | 2 (src + include_private) |
| Internal headers | 5 |
| Private interface headers | 5 |
| Backend source files (.cpp) | 5 |
| Test files | 3 |
| CMake targets | 1 new (kivo_p6c_ffmpeg_decode_tests) |
| Namespace | kivo::video::audio_plane::backend::decode::ffmpeg |

### 1.3 No Public Headers

P6C creates **ZERO** public headers. All FFmpeg types are private:
- `AVCodecContext*`, `AVFrame*`, `AVPacket*` exist ONLY in `backend/src/.../internal/` and `backend/include_private/.../`
- Public P6A contracts (`AudioDecodeBackendContract`, `DecodedAudioFrame`, etc.) are consumed, not modified
- Architecture guard `BackendPrivate` mode scans `backend/src/video/audio_plane/` for forbidden path leaks

---

## 2. REPO INVENTORY (P6C-specific)

### 2.1 P6A Decode Contracts (FROZEN — consumed by P6C, NOT modifiable)

| P6A Header | Type | P6C Consumes |
|------------|------|--------------|
| `decode/audio_decode_backend_contract.hpp` | AudioDecodeBackendContract | Backend contract (supports_drain, supports_flush, max_queue_size) |
| `decode/audio_decode_backend_lifecycle.hpp` | AudioDecodeBackendState (16 states), AudioDecodeBackendLifecycle | State machine + is_terminal/is_timed_out |
| `decode/audio_decode_backend_state.hpp` | AudioDecodeBackendStateSnapshot | State snapshot for evidence |
| `decode/audio_decode_request.hpp` | AudioDecodeRequest | Decode request (packet_handle_id, max_frames, pipeline_generation) |
| `decode/audio_decode_result.hpp` | AudioDecodeStatus, AudioDecodeResult | Decode result (status, frames_decoded, frame_handle_id) |
| `decode/decoded_audio_frame.hpp` | DecodedAudioFrame | Output frame (sample_rate, channels, bit_depth, pts, pool_slot_id) |
| `decode/audio_decoder_flush_policy.hpp` | AudioDecoderFlushPolicy | Flush policy (cancel_pending, release_handles, timeout) |
| `decode/audio_decoder_drain_policy.hpp` | AudioDecoderDrainPolicy | Drain policy (timeout, init timeouts, evidence) |
| `decode/audio_decode_evidence.hpp` | DecodeEvidenceKind, AudioDecodeEvidence | Evidence kind enum + record |

### 2.2 P6B Fake Decode Runtime (FROZEN — interface pattern reference, NOT modifiable)

| P6B Header | Class | P6C Reference |
|------------|-------|---------------|
| `runtime/decode/fake_decode_backend.hpp` | FakeDecodeBackend | Interface pattern: init/submit_packet/receive_frame/transition_to |
| `runtime/decode/fake_decode_lifecycle_controller.hpp` | FakeDecodeLifecycleController | Adjacency map pattern, cancel/supersede |

**Key insight**: P6C implements the SAME interface pattern as P6B fake, but with REAL FFmpeg calls instead of deterministic counter-based generation.

### 2.3 Existing FFmpeg Infrastructure (pre-P6C)

| Component | Location | P6C Impact |
|-----------|----------|------------|
| FFmpeg adapter (P1/P2) | `backend/src/third_party_adapter/ffmpeg_adapter/` | P6C creates its OWN isolated adapter in `decode/ffmpeg/`, does NOT reuse legacy |
| CMake FFmpeg integration | `backend/CMakeLists.txt` lines 109-117 | P6C adds new target with `KIVO_ENABLE_FFMPEG=ON` guard |
| FFmpeg SDK | `C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1` | P6C links avcodec, avutil |

### 2.4 V2.0 Design References

| Section | Content | P6C Relevance |
|---------|---------|---------------|
| §2 | "自研音频核心" — codec backend 可替换 | P6C = FFmpeg backend, one of replaceable backends |
| §4.1 | P6 负责 Decode backend lifecycle | P6C implements real lifecycle |
| §5.2 | FFmpeg private backend root: `backend/src/video/audio_plane/decode/ffmpeg/internal/` | P6C directory structure |
| §11 | Decode Backend Lifecycle State Machine (16 states) | P6C implements all 16 states + transitions |
| §12 | Encoded Passthrough Parse Path (separate from decode) | P6C does NOT implement passthrough (P6E scope) |
| §33 | Codec Private Data / DRM / Protected Audio | P6C handles codec private data, NOT DRM decrypt |

---

## 3. P6C SCOPE: 15-POINT PLANNING INVENTORY

### 3.1 FFmpeg Codec Context Guard (C1)

**Problem**: AVCodecContext must be properly created, configured, and freed. No leaks.

**P6C plan**:
- `ffmpeg_codec_context_guard.hpp` in `internal/`:
  - RAII wrapper: constructor opens codec, destructor calls `avcodec_free_context`
  - `get() → AVCodecContext*` (internal use only)
  - `is_valid() → bool`
  - Move-only (delete copy)

### 3.2 FFmpeg Frame Guard (C2)

**Problem**: AVFrame must be allocated and freed correctly.

**P6C plan**:
- `ffmpeg_frame_guard.hpp` in `internal/`:
  - RAII wrapper: constructor `av_frame_alloc`, destructor `av_frame_free`
  - `get() → AVFrame*`
  - `reset()` — unref + reuse
  - Move-only

### 3.3 FFmpeg Packet Guard (C3)

**Problem**: AVPacket must be allocated, filled, and freed correctly.

**P6C plan**:
- `ffmpeg_packet_guard.hpp` in `internal/`:
  - RAII wrapper: constructor `av_packet_alloc`, destructor `av_packet_free`
  - `get() → AVPacket*`
  - `set_data(bytes, size)` — copies data into packet
  - Move-only

### 3.4 FFmpeg Codec Registry (C4)

**Problem**: P6A AudioCodecId (int32_t) must map to FFmpeg AVCodecID.

**P6C plan**:
- `ffmpeg_codec_registry.hpp` in `internal/`:
  - `to_avcodec_id(int32_t p6_codec_id) → AVCodecID`
  - Mapping: AAC→AV_CODEC_ID_AAC, MP3→AV_CODEC_ID_MP3, AC3→AV_CODEC_ID_AC3, etc.
  - `is_supported(int32_t p6_codec_id) → bool`
  - `requires_codec_private_data(AVCodecID) → bool` (e.g., AAC needs AudioSpecificConfig)

### 3.5 FFmpeg Decode Configuration (C5)

**Problem**: avcodec_open2 requires configuration parameters.

**P6C plan**:
- `ffmpeg_decode_config.hpp` in `internal/`:
  - `FFmpegDecodeConfig` struct: time_base, channel_layout, sample_rate, bit_depth
  - `apply_to_context(AVCodecContext*, const FFmpegDecodeConfig&)` — sets avcodec parameters
  - `extract_from_context(AVCodecContext*) → FFmpegDecodeConfig` — reads post-init parameters

### 3.6 FFmpeg Decode Backend (C6)

**Problem**: Real decode backend implementing P6A contract interface pattern.

**P6C plan**:
- `FFmpegDecodeBackend` in `ffmpeg_decode_backend.hpp` (private) + `.cpp`:
  - `init(codec_id, codec_private_data_ref) → Expected<void, InitError>`
    - Creates AVCodecContext via guard
    - Sets codec parameters from config
    - Calls `avcodec_open2`
    - Transitions: NotCreated → Created → Opening → Ready
    - On failure: → FailedRecoverable or FailedFatal
  - `submit_packet(AudioDecodeRequest) → Expected<void, DecodeReject>`
    - Converts packet handle to AVPacket via guard
    - Calls `avcodec_send_packet`
    - Transitions: Ready → Decoding
  - `receive_frame() → Expected<DecodedAudioFrame, DecodeStatus>`
    - Calls `avcodec_receive_frame` into AVFrame guard
    - Converts AVFrame → DecodedAudioFrame via converter
    - Returns WouldBlock if EAGAIN, Eos if AVERROR_EOF
    - TimedOut if deadline exceeded
  - `flush() → Expected<void, FlushError>`
    - Calls `avcodec_flush_buffers`
    - Transitions: Decoding → Flushing → Ready
  - `drain() → Expected<void, DrainError>`
    - Sends NULL packet (`avcodec_send_packet(ctx, NULL)`)
    - Receives until Eos
    - Transitions: Decoding → Draining → Ready
  - `close() → void`
    - Releases all guards
    - Transitions: Any → Closed
  - `transition_to(state) → Expected<void, TransitionError>` — validates §11 adjacency

### 3.7 FFmpeg Decode Lifecycle Controller (C7)

**Problem**: Real lifecycle transitions with timeout monitoring.

**P6C plan**:
- `FFmpegDecodeLifecycleController` in `ffmpeg_decode_lifecycle.hpp` + `.cpp`:
  - Same adjacency map as P6B fake (§11)
  - `transition(from, to) → Expected<void, LifecycleTransitionError>`
  - `check_timeout(current_time_ms) → Optional<TimedOut>` — real deadline monitoring
  - `cancel(reason) → void` — releases FFmpeg handles
  - `supersede(new_generation) → void` — blocks future frame publication
  - Timeout thresholds: normal=3000ms, live=5000ms, protected=policy-defined

### 3.8 FFmpeg Frame Converter (C8)

**Problem**: AVFrame (planar/interleaved, various formats) → P6 DecodedAudioFrame.

**P6C plan**:
- `FFmpegFrameConverter` in `ffmpeg_frame_converter.hpp` + `.cpp`:
  - `convert(const AVFrame* frame, uint64_t pool_slot_id) → DecodedAudioFrame`
    - Extracts: sample_rate (frame->sample_rate), channels (frame->ch_layout.nb_channels)
    - Extracts: pts (frame->pts), format (frame->format → bit_depth)
    - Assigns: pool_slot_id (from frame pool)
  - `detect_format_changed(const AVFrame* frame, const FFmpegDecodeConfig& prev) → bool`
    - Compares sample_rate, channels, format, channel_layout
    - Returns true if FormatChanged should be emitted
  - `extract_sample_format(AVSampleFormat) → int32_t` (bit_depth + planar flag)

### 3.9 Codec Private Data Adapter (C9)

**Problem**: P6A AudioCodecPrivateDataRef (uint64_t) must provide bytes for avcodec extradata.

**P6C plan**:
- `FFmpegCodecPrivateDataAdapter` in `ffmpeg_codec_private_data.hpp` + `.cpp`:
  - `load(uint64_t codec_private_data_ref) → Expected<std::vector<uint8_t>, LoadError>`
    - Resolves ref to actual bytes (from P6 data handle manager)
    - Returns owned bytes (no borrowed span)
  - `apply_to_context(AVCodecContext* ctx, const std::vector<uint8_t>& data)`
    - Sets `ctx->extradata` and `ctx->extradata_size`
    - Allocates with AV_INPUT_BUFFER_PADDING_SIZE
  - **Security**: Never logs private data content, never stores in evidence (P6A §33)

### 3.10 Decode Evidence Emitter (C10)

**Problem**: Real decode evidence must be emitted for all lifecycle events.

**P6C plan**:
- `FFmpegDecodeEvidenceEmitter` in `ffmpeg_decode_evidence_emitter.hpp` + `.cpp`:
  - `emit_init_ok(codec_id, timestamp_ms) → AudioDecodeEvidence`
  - `emit_init_failed(codec_id, error_code, timestamp_ms) → AudioDecodeEvidence`
  - `emit_decode_ok(frame_id, pts, timestamp_ms) → AudioDecodeEvidence`
  - `emit_decode_failed(packet_id, error_code, timestamp_ms) → AudioDecodeEvidence`
  - `emit_timed_out(deadline_ms, timestamp_ms) → AudioDecodeEvidence`
  - `emit_flushed(timestamp_ms) → AudioDecodeEvidence`
  - `emit_drained(frame_count, timestamp_ms) → AudioDecodeEvidence`
  - `emit_format_changed(old_config, new_config, timestamp_ms) → AudioDecodeEvidence`
  - `emit_superseded(old_gen, new_gen, timestamp_ms) → AudioDecodeEvidence`

### 3.11 TimedOut Closure (C11)

**Problem**: V2.0 §11 mandates TimedOut state must close the loop.

**P6C plan**:
- Real timeout monitoring in `FFmpegDecodeLifecycleController`:
  - `set_deadline_ms(uint32_t ms)` — records deadline
  - `check_timeout(uint64_t current_ms) → Optional<AudioDecodeBackendState::TimedOut>`
  - On timeout: transition to TimedOut, emit evidence, block frame publication
  - Recovery: TimedOut → Resetting → Ready (via `avcodec_flush_buffers` + re-init)

### 3.12 FormatChanged Detection (C12)

**Problem**: V2.0 §26 mandates FormatChanged detection and handling.

**P6C plan**:
- In `FFmpegDecodeBackend::receive_frame()`:
  - After `avcodec_receive_frame`, call `FFmpegFrameConverter::detect_format_changed()`
  - If format changed: transition to FormatChanging
  - Emit `FormatChangedEvidence`
  - Block old-generation frames, require pipeline format barrier (P6A §26)
  - Transition: FormatChanging → Ready (after rebuild) or FailedFatal

### 3.13 Cancellation + Supersede (C13)

**Problem**: V2.0 §11 mandates cancellation and supersede must release handles.

**P6C plan**:
- `cancel(CancelReason)`:
  - Aborts in-flight `avcodec_send_packet` / `avcodec_receive_frame` (via interrupt callback)
  - Releases AVCodecContext, AVFrame, AVPacket guards
  - Transitions to Closed
- `supersede(new_generation)`:
  - Blocks all future `receive_frame` publication
  - Emits `SupersededEvidence`
  - Does NOT immediately close (allows drain of old frames if policy permits)

### 3.14 Flush + Drain (C14)

**Problem**: V2.0 §11 mandates flush and drain behavior.

**P6C plan**:
- `flush()`:
  - Calls `avcodec_flush_buffers(ctx)`
  - Cancels pending decode
  - Releases P6 handles per `AudioDecoderFlushPolicy`
  - Transitions: Decoding → Flushing → Ready
  - Timeout: `flush_timeout_ms` (default 1000ms)
- `drain()`:
  - Sends NULL packet: `avcodec_send_packet(ctx, NULL)`
  - Loops `avcodec_receive_frame` until AVERROR_EOF
  - Emits DrainedEvidence with frame count
  - Transitions: Decoding → Draining → Ready
  - Timeout: `drain_timeout_ms` (default 3000ms)

### 3.15 FFmpeg Interrupt Callback (C15)

**Problem**: `avcodec_send_packet` / `avcodec_receive_frame` can block. Need interruptible.

**P6C plan**:
- `ffmpeg_decode_config.hpp` includes interrupt callback setup:
  - `set_interrupt_callback(AVCodecContext*, std::function<int()> cb)`
  - Callback returns 1 to abort blocking FFmpeg call
  - Used by `cancel()` to abort in-flight decode
  - Used by timeout monitoring to abort long-running decode

---

## 4. P6C EXACT ALLOWED FILES

### 4.1 Allowed Directories (backend source — real FFmpeg)

| # | Directory | Purpose |
|---|-----------|---------|
| 1 | `backend/src/video/audio_plane/decode/ffmpeg/` | P6C FFmpeg decode backend source root |
| 2 | `backend/src/video/audio_plane/decode/ffmpeg/internal/` | FFmpeg internal RAII guards + registry |
| 3 | `backend/include_private/video/audio_plane/decode/ffmpeg/` | P6C private interface headers |

**3 directories, 15 files (5 internal + 5 private + 5 source)**

### 4.2 Allowed Directories (tests)

| # | Directory | Purpose |
|---|-----------|---------|
| T1 | `backend/tests/video/audio_plane/p6c_ffmpeg_decode_tests/` | P6C FFmpeg decode tests |

**1 test directory, 3 test files**

### 4.3 Allowed CMake Files

| # | File | Purpose |
|---|------|---------|
| 1 | `backend/CMakeLists.txt` | Add `kivo_p6c_ffmpeg_decode_tests` target (guarded by `KIVO_ENABLE_FFMPEG`) |

### 4.4 Allowed Docs

| # | File | Purpose |
|---|------|---------|
| 1 | `docs/roadmap/p6c_planning_draft.md` | THIS DOCUMENT |

---

## 5. P6C FORBIDDEN FILES

| # | Forbidden Item |
|---|---------------|
| F-01 | ❌ Any modification to P6A contract headers (298 headers FROZEN) |
| F-02 | ❌ Any modification to P6B runtime headers (22 headers FROZEN) |
| F-03 | ❌ Any public header in `include/kivo/video/audio_plane/` (P6C = private backend only) |
| F-04 | ❌ Any real WASAPI call (P6D scope) |
| F-05 | ❌ Any real resampler call (P6E scope) |
| F-06 | ❌ Any real passthrough parse (P6E scope) |
| F-07 | ❌ Any real DSP chain (P6E scope) |
| F-08 | ❌ Any real clock/sync runtime (P6E scope) |
| F-09 | ❌ P6D/P6E/P6F/P6G/P6H code |
| F-10 | ❌ Any file outside allowed directories |
| F-11 | ❌ P4/P5/P8 file modification |
| F-12 | ❌ Qt / QML / D3D11 / DXGI |
| F-13 | ❌ Public exposure of FFmpeg types (AVFrame/AVPacket/AVCodecContext/AVCodecParameters/SwrContext) |
| F-14 | ❌ Codec private data in evidence/crash dump (P6A §33) |
| F-15 | ❌ DRM key storage / DRM decrypt (P6 handles private data only, NOT DRM) |
| **Allow** | ✅ Private backend .cpp files ARE allowed (`backend/src/video/audio_plane/decode/ffmpeg/`) |
| **Allow** | ✅ Private interface .hpp files ARE allowed (`backend/include_private/video/audio_plane/decode/ffmpeg/`) |
| **Allow** | ✅ Test .cpp files ARE allowed (`backend/tests/video/audio_plane/p6c_ffmpeg_decode_tests/`) |

---

## 6. P6C EXIT CRITERIA (15 ITEMS)

| # | Criterion | Coverage | Status |
|---|-----------|----------|--------|
| E1 | FFmpeg codec context RAII guard | §3.1 (C1) | ✅ DESIGNED |
| E2 | FFmpeg frame RAII guard | §3.2 (C2) | ✅ DESIGNED |
| E3 | FFmpeg packet RAII guard | §3.3 (C3) | ✅ DESIGNED |
| E4 | P6 AudioCodecId → AVCodecID mapping | §3.4 (C4) | ✅ DESIGNED |
| E5 | FFmpeg decode configuration | §3.5 (C5) | ✅ DESIGNED |
| E6 | FFmpegDecodeBackend (init/submit/receive/flush/drain/close) | §3.6 (C6) | ✅ DESIGNED |
| E7 | FFmpegDecodeLifecycleController (real transitions + timeout) | §3.7 (C7) | ✅ DESIGNED |
| E8 | AVFrame → DecodedAudioFrame converter | §3.8 (C8) | ✅ DESIGNED |
| E9 | Codec private data adapter (no leak) | §3.9 (C9) | ✅ DESIGNED |
| E10 | Decode evidence emitter (9 kinds) | §3.10 (C10) | ✅ DESIGNED |
| E11 | TimedOut state closure (real deadline) | §3.11 (C11) | ✅ DESIGNED |
| E12 | FormatChanged detection + handling | §3.12 (C12) | ✅ DESIGNED |
| E13 | Cancellation + supersede (handle release) | §3.13 (C13) | ✅ DESIGNED |
| E14 | Flush + drain (avcodec_flush_buffers + NULL packet) | §3.14 (C14) | ✅ DESIGNED |
| E15 | FFmpeg interrupt callback (cancel blocking decode) | §3.15 (C15) | ✅ DESIGNED |

**Coverage: 15/15 DESIGNED (0 NOT COVERED)**

---

## 7. ARCHITECTURE GUARD COVERAGE

### 7.1 PublicHeadersStrict Mode (unchanged)

- Scans `include/kivo/video/audio_plane/**/*.hpp` (P6A contracts + P6B runtime = 320 headers)
- P6C creates ZERO public headers → no new scan targets
- Existing guard continues to PASS

### 7.2 BackendPrivate Mode (MUST be extended for P6C)

- Current: scans `backend/src/video/audio_plane/` (does not exist in P6A/P6B)
- P6C creates: `backend/src/video/audio_plane/decode/ffmpeg/`
- Guard MUST scan this directory for:
  - Forbidden path segments: `/wasapi/`, `/qt/` (NOT `/ffmpeg/` — FFmpeg is allowed here)
  - Public type leaks: FFmpeg types must NOT appear in `backend/include_private/` headers that are included by public contracts
- **Guard extension plan**: `BackendPrivate` mode scan path includes `backend/src/video/audio_plane/` and `backend/include_private/video/audio_plane/`

### 7.3 Forbidden Token Rules for P6C

| Location | FFmpeg types allowed? | WASAPI/Qt types allowed? |
|----------|----------------------|--------------------------|
| `include/kivo/video/audio_plane/` (public) | ❌ NO | ❌ NO |
| `backend/include_private/video/audio_plane/decode/ffmpeg/` (private interface) | ✅ YES (FFmpeg types in declarations) | ❌ NO |
| `backend/src/video/audio_plane/decode/ffmpeg/internal/` (internal) | ✅ YES | ❌ NO |
| `backend/src/video/audio_plane/decode/ffmpeg/` (source) | ✅ YES | ❌ NO |
| `backend/tests/video/audio_plane/p6c_ffmpeg_decode_tests/` (tests) | ✅ YES | ❌ NO |

---

## 8. CMake TARGET STRUCTURE

```cmake
# P6C: FFmpeg Decode Backend Tests (guarded by KIVO_ENABLE_FFMPEG)
if(KIVO_ENABLE_FFMPEG)
  add_executable(kivo_p6c_ffmpeg_decode_tests
    tests/video/audio_plane/p6c_ffmpeg_decode_tests/ffmpeg_decode_backend_tests.cpp
    tests/video/audio_plane/p6c_ffmpeg_decode_tests/ffmpeg_frame_converter_tests.cpp
    tests/video/audio_plane/p6c_ffmpeg_decode_tests/ffmpeg_decode_evidence_tests.cpp
    src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_backend.cpp
    src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_lifecycle.cpp
    src/video/audio_plane/decode/ffmpeg/ffmpeg_frame_converter.cpp
    src/video/audio_plane/decode/ffmpeg/ffmpeg_codec_private_data.cpp
    src/video/audio_plane/decode/ffmpeg/ffmpeg_decode_evidence_emitter.cpp
  )
  target_include_directories(kivo_p6c_ffmpeg_decode_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/backend/include_private
    ${CMAKE_SOURCE_DIR}/backend/src/video/audio_plane/decode/ffmpeg/internal
    ${FFMPEG_ROOT}/include
  )
  target_compile_features(kivo_p6c_ffmpeg_decode_tests PRIVATE cxx_std_23)
  target_link_libraries(kivo_p6c_ffmpeg_decode_tests PRIVATE
    kivo_audio_plane_contracts
    avcodec avutil
  )
  add_test(NAME kivo_p6c_ffmpeg_decode_tests COMMAND kivo_p6c_ffmpeg_decode_tests)
  set_tests_properties(kivo_p6c_ffmpeg_decode_tests PROPERTIES TIMEOUT 60)
endif()
```

**Link Rules**:
- P6C tests link `kivo_audio_plane_contracts` (P6A) + `avcodec` + `avutil`
- P6C does NOT link `kivo_cinema_engine` (legacy)
- P6C does NOT link WASAPI / D3D11 / Qt
- P6C does NOT link `swresample` (resampler is P6E scope)

---

## 9. RISKS / BLOCKERS

| # | Risk | Severity | Mitigation |
|---|------|----------|-----------|
| R1 | FFmpeg API version differences (n7.1 vs future) | MEDIUM | P6C targets FFmpeg n7.1 (pinned in CMakePresets). API changes require P6C update. |
| R2 | avcodec_send_packet can block on corrupt data | MEDIUM | Interrupt callback (C15) allows cancellation. Timeout monitoring (C11) catches long blocks. |
| R3 | Codec private data security (§33) | HIGH | P6C never logs, stores in evidence, or exposes private data. RAII guards ensure no leak. |
| R4 | AVFrame planar→interleaved conversion complexity | LOW | P6C converts to P6 DecodedAudioFrame (interleaved assumption). Planar format detection via AVSampleFormat. |
| R5 | FormatChanged mid-stream may require codec reinit | MEDIUM | P6C detects format change (C12), emits evidence, transitions to FormatChanging. Reinit via avcodec_flush + avcodec_open2. |
| R6 | Test requires real FFmpeg SDK + real codec data | MEDIUM | Tests use `KIVO_ENABLE_FFMPEG=ON` + `KIVO_ENABLE_REAL_MEDIA_TESTS=ON`. Fake test data (synthetic packets) where possible. Real media tests are DEFERRED to lab. |

---

## 10. WHETHER READY FOR P6C IMPLEMENTATION

| Check | Result |
|-------|--------|
| P6A contract layer closed? | ✅ 44/44 gates PASS, commit 3d50ec3 |
| P6B fake runtime closed? | ✅ 22/22 exit criteria PASS, commit 874a725 |
| V2.0 design lock confirmed? | ✅ USER CONFIRMED |
| All 15 planning items covered? | ✅ 15/15 DESIGNED |
| P6A contracts consumed (not modified)? | ✅ FROZEN |
| P6B runtime consumed (not modified)? | ✅ FROZEN |
| No forbidden scope creep? | ✅ NO WASAPI/resampler/DSP/passthrough |
| All forbidden files listed? | ✅ 15 hard stops |
| Exit criteria measurable? | ✅ 15 items, each mapped to C1-C15 |
| FFmpeg SDK available? | ✅ C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1 |

### READY FOR P6C IMPLEMENTATION: YES ⚠️ CONDITIONAL

**Conditions**:
1. User must explicitly approve this Planning Draft before any file creation.
2. P6A contract headers + P6B runtime headers must remain FROZEN.
3. FFmpeg SDK must be present at configured path.
4. `KIVO_ENABLE_FFMPEG=ON` required for build/test.

---

## 11. IMMEDIATE NEXT STEPS (AFTER APPROVAL)

1. Create directory scaffold (3 dirs: src + include_private + tests)
2. Add CMake target: `kivo_p6c_ffmpeg_decode_tests` (guarded by `KIVO_ENABLE_FFMPEG`)
3. Create internal RAII guards (C1-C3) + codec registry (C4) + config (C5)
4. Create FFmpegDecodeBackend (C6) + lifecycle controller (C7)
5. Create frame converter (C8) + codec private data adapter (C9)
6. Create evidence emitter (C10)
7. Implement TimedOut (C11) + FormatChanged (C12) + cancellation (C13) + flush/drain (C14) + interrupt (C15)
8. Create test files (3 files, C1-C15)
9. Run Gate Review: compile + CTest + forbidden token scan + architecture guard

**Do NOT enter P6D/P6E. Do NOT enter P7/P8.**

---

**END OF P6C PLANNING DRAFT 001**
**Status**: DRAFT_FOR_REVIEW — awaiting user confirmation

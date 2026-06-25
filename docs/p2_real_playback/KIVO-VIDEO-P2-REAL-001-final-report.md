# KIVO-VIDEO-P2-REAL-001-REPAIR-001 — FFmpeg ON Real Runtime Proof

## Classification

**KIVO-VIDEO-P2-REAL-001-REPAIR-001 FFMPEG_ON_REAL_RUNTIME_PROOF COMPLETED_SUCCESSFULLY**

## Summary

FFmpeg adapter type boundary repaired. KIVO_ENABLE_FFMPEG=ON compiles and links with real FFmpeg SDK. All 70 tests pass (FFmpeg ON) and 67 tests pass (FFmpeg OFF).

## Changes

### Files Modified (8)

1. **`backend/include/kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp`**
   - Replaced FFmpeg forward declarations with opaque handle types
   - Added `FfmpegFormatHandle`, `FfmpegCodecHandle`, `FfmpegPacketInfo`, `FfmpegReadResult`, `FfmpegFrameResult`, `FfmpegCodecInfo`, `FfmpegSendStatus`, `FfmpegSendResult`
   - Expanded `FfmpegAdapter` class with full API surface

2. **`backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.cpp`**
   - Complete implementation of adapter methods using real FFmpeg 7.1 API
   - Fixed FFmpeg 7.1 API changes: removed `AVFrame::dts`, replaced `av_get_channel_layout_string` with `av_channel_layout_describe`

3. **`backend/src/demux_core/real_probe_runtime.cpp`**
   - Removed all FFmpeg includes and `static_cast<AVFormatContext*>`
   - Delegates to `FfmpegAdapter` via opaque handles

4. **`backend/src/demux_core/real_demux_runtime.cpp`**
   - Removed all FFmpeg includes and `#ifdef KIVO_ENABLE_FFMPEG` blocks
   - Uses `FfmpegAdapter` methods exclusively

5. **`backend/src/decode_core/real_software_decode_runtime.cpp`**
   - Removed all FFmpeg includes and `#ifdef KIVO_ENABLE_FFMPEG` blocks
   - Uses `FfmpegAdapter` methods exclusively

6. **`backend/CMakeLists.txt`**
   - Added FFmpeg find/link configuration with `FFMPEG_ROOT` variable
   - PUBLIC link directives for static library propagation

7. **`CMakePresets.json`**
   - Added `FFMPEG_ROOT` cache variable to `vs-debug-ffmpeg` and `ninja-debug-ffmpeg` presets

8. **`backend/tests/integration_local/real_demux_decode_integration_test.cpp`**
   - Fixed `error_code` → `error_message` (DemuxResult field name)

## FFmpeg SDK Setup

- **Source**: BtbN GitHub prebuilt `ffmpeg-n7.1-latest-win64-gpl-shared-7.1`
- **Location**: `C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1`
- **Import libs**: Generated from DLL exports using `dumpbin /exports` → `.def` → `lib.exe /def:file.def /machine:x64`
- **Runtime DLLs**: Copied to build output directory

## Verification Results

| Check | Result |
|-------|--------|
| FFmpeg OFF configure | PASS |
| FFmpeg OFF build | PASS |
| FFmpeg OFF CTest | 67/67 PASS |
| FFmpeg ON configure | PASS (FFmpeg SDK root found) |
| FFmpeg ON build | 70 targets PASS |
| FFmpeg ON CTest | 70/70 PASS |
| FFmpeg boundary gate (public headers) | PASS (only comment references, no code leakage) |
| FFmpeg types in adapter header | Opaque handles only (no AVFormatContext etc.) |

## Key Design Decisions

1. **Opaque Handle Pattern**: Public headers expose `void*` wrappers instead of FFmpeg forward declarations
2. **Single adapter file**: Only `ffmpeg_adapter.cpp` includes real FFmpeg headers (guarded by `#ifdef KIVO_ENABLE_FFMPEG`)
3. **PUBLIC link propagation**: Static library requires PUBLIC link directives for consumers to resolve FFmpeg symbols
4. **FFmpeg 7.1 API adaptation**: Removed deprecated `AVFrame::dts` and `av_get_channel_layout_string`

## Boundary Gate Analysis

FFmpeg type references found in public headers are **comment-only** (explaining the opaque pattern):
- `real_probe_runtime.hpp`: "// Probe an already-opened AVFormatContext (passed as void* to avoid FFmpeg header dependency)"
- `real_demux_runtime.hpp`: "// No AVFormatContext leaks into business code" + "void* format_context_ // AVFormatContext*"
- `real_software_decode_runtime.hpp`: "// No AVCodecContext leaks into business code" + "void* codec_context_ // AVCodecContext*"

No actual FFmpeg types appear in public header declarations or function signatures.

# P2-REAL-001: Local File Real Probe / Demux / Decode Vertical Slice

## Overview

This document describes the first real FFmpeg-based vertical slice for the Kivo Video P2 backend, implementing the core media processing pipeline: local file → byte stream → container probe → demux → software decode → decoded frame contract.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Business Core Layer                       │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ LocalFileByte │  │ RealProbe    │  │ RealDemux        │  │
│  │ Stream        │  │ Runtime      │  │ Runtime          │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────────┘  │
│         │                  │                  │              │
│  ┌──────┴──────────────────┴──────────────────┴──────────┐  │
│  │               FfmpegAdapter Boundary                  │  │
│  │         (third_party_adapter/ffmpeg_adapter/)         │  │
│  └──────────────────────────┬────────────────────────────┘  │
│                             │                               │
│  ┌──────────────────────────┴────────────────────────────┐  │
│  │              FFmpeg Library (libavformat/libavcodec)   │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Key Design Decisions

### 1. Adapter Boundary Pattern
FFmpeg types (`AVFormatContext`, `AVCodecContext`, `AVPacket`, `AVFrame`) are **only** present in `third_party_adapter/ffmpeg_adapter/`. Business code interacts through:
- `void*` opaque pointers in `RealDemuxRuntime` and `RealSoftwareDecodeRuntime`
- `FfmpegAdapter` static methods as the single translation unit containing FFmpeg headers

### 2. Conditional Compilation
All FFmpeg code is guarded by `#ifdef KIVO_ENABLE_FFMPEG`:
- **Default OFF**: Project builds without FFmpeg dependency
- **Stub implementations**: Return false/nullptr when disabled, ensuring no linking errors
- **Full implementations**: Real FFmpeg calls when enabled

### 3. Environment-Variable Driven Testing
Integration tests use environment variables (`KIVO_SAMPLE_H264_AAC_MP4`, etc.) to locate sample media files:
- Tests **SKIP** if variables are not set (CI-friendly)
- Tests **SKIP** if FFmpeg is not enabled
- No sample media files are committed to the repository

## File Inventory

### Byte Stream Core (`byte_stream_core/`)
| File | Purpose |
|------|---------|
| `local_file_byte_stream.hpp` | Abstract virtual interface |
| `local_file_byte_stream_impl.hpp` | Concrete implementation (header) |
| `local_file_byte_stream_impl.cpp` | Concrete implementation (source) |
| `local_file_byte_stream_open_request.hpp` | Open request contract |
| `local_file_byte_stream_open_result.hpp` | Open result contract |

### FFmpeg Adapter (`third_party_adapter/ffmpeg_adapter/`)
| File | Purpose |
|------|---------|
| `ffmpeg_adapter.hpp` | Public interface with forward-declared FFmpeg types |
| `ffmpeg_adapter.cpp` | Full implementation with conditional compilation |

### Demux Core (`demux_core/`)
| File | Purpose |
|------|---------|
| `real_probe_runtime.hpp` | Real probe runtime (StreamInfo, ProbeResult) |
| `real_probe_runtime.cpp` | FFmpeg-based container probing |
| `real_demux_runtime.hpp` | Real demux runtime (opaque format context) |
| `real_demux_runtime.cpp` | FFmpeg-based packet reading |

### Decode Core (`decode_core/`)
| File | Purpose |
|------|---------|
| `real_software_decode_runtime.hpp` | Real software decode runtime (opaque codec context) |
| `real_software_decode_runtime.cpp` | FFmpeg-based frame decoding |

### Tests
| File | Type |
|------|------|
| `demux_core/real_probe_runtime_test.cpp` | Unit test (10 tests) |
| `demux_core/real_demux_runtime_test.cpp` | Unit test (11 tests) |
| `decode_core/real_software_decode_runtime_test.cpp` | Unit test (12 tests) |
| `integration_local/local_file_byte_stream_integration_test.cpp` | Integration (4 tests) |
| `integration_local/real_probe_integration_test.cpp` | Integration (4 tests) |
| `integration_local/real_demux_decode_integration_test.cpp` | Integration (4 tests) |

## Build Configuration

### CMake Options
```cmake
option(KIVO_ENABLE_FFMPEG "Enable FFmpeg-based real media processing" OFF)
option(KIVO_ENABLE_REAL_MEDIA_TESTS "Enable integration tests requiring real media samples" OFF)
```

### Presets
- `ninja-debug`: Default build (no FFmpeg)
- `ninja-debug-ffmpeg`: FFmpeg-enabled build with integration tests

### Build Commands
```powershell
# Default build (no FFmpeg)
cmake --preset ninja-debug
cmake --build --preset ninja-debug

# FFmpeg-enabled build
cmake --preset ninja-debug-ffmpeg
cmake --build --preset ninja-debug-ffmpeg

# Run tests
ctest --preset ninja-debug
```

## Sample Media Setup

To run integration tests with real media:

1. Set environment variable:
   ```powershell
   $env:KIVO_SAMPLE_H264_AAC_MP4 = "C:\path\to\sample.mp4"
   ```

2. Build with FFmpeg:
   ```powershell
   cmake --preset ninja-debug-ffmpeg
   cmake --build --preset ninja-debug-ffmpeg
   ```

3. Run tests:
   ```powershell
   ctest --preset ninja-debug
   ```

## Forbidden Token Boundary

The following tokens are **only** allowed in `third_party_adapter/ffmpeg_adapter/`:
- `AVFormatContext`, `AVCodecContext`, `AVPacket`, `AVFrame`
- `AVCodecParameters`, `AVCodec`, `AVStream`
- `avformat_open_input`, `avformat_close_input`, `av_read_frame`
- `avcodec_send_packet`, `avcodec_receive_frame`
- `avcodec_find_decoder`, `avcodec_open2`

Business code must never include FFmpeg headers directly.

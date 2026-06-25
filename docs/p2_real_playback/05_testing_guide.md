# Testing Guide for P2-REAL-001

## Test Categories

### 1. Unit Tests (Always Run)
Verify structural correctness, default values, and lifecycle behavior.

| Test File | Tests | Coverage |
|-----------|-------|----------|
| `real_probe_runtime_test.cpp` | 10 | ProbeResult/StreamInfo structure, availability checks, error handling |
| `real_demux_runtime_test.cpp` | 11 | Demux lifecycle, move semantics, error states |
| `real_software_decode_runtime_test.cpp` | 12 | Decode lifecycle, codec opening, move semantics |

### 2. Integration Tests (Require Environment Variables)
Verify real file I/O and FFmpeg-based processing.

| Test File | Tests | Requirements |
|-----------|-------|--------------|
| `local_file_byte_stream_integration_test.cpp` | 4 | `KIVO_SAMPLE_H264_AAC_MP4` |
| `real_probe_integration_test.cpp` | 4 | `KIVO_SAMPLE_H264_AAC_MP4`, FFmpeg enabled |
| `real_demux_decode_integration_test.cpp` | 4 | `KIVO_SAMPLE_H264_AAC_MP4`, FFmpeg enabled |

## Running Tests

### Default Build (No FFmpeg)
```powershell
cmake --preset ninja-debug
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```
**Result**: 33 unit tests pass. Integration tests registered but skip without sample files.

### FFmpeg-Enabled Build
```powershell
cmake --preset ninja-debug-ffmpeg
cmake --build --preset ninja-debug-ffmpeg
ctest --preset ninja-debug
```
**Result**: 33 unit tests pass. Integration tests run if sample files are provided.

### With Sample Media
```powershell
# Set sample file
$env:KIVO_SAMPLE_H264_AAC_MP4 = "C:\media\sample.mp4"

# Run all tests
ctest --preset ninja-debug --output-on-failure
```
**Result**: 33 unit + 12 integration tests pass.

## Test Behavior by FFmpeg Availability

### When FFmpeg is Disabled
| Test | Behavior |
|------|----------|
| `is_ffmpeg_available()` | Returns `false` |
| `supported_*()` | Returns empty vectors |
| `probe_file()` | Returns `ffmpeg_not_available` error |
| `decode()` | Returns `ffmpeg_not_available` error |
| Unit tests | Test stub/error behavior |
| Integration tests | Skip with "FFmpeg not enabled" |

### When FFmpeg is Enabled
| Test | Behavior |
|------|----------|
| `is_ffmpeg_available()` | Returns `true` |
| `supported_*()` | Returns real format/codec lists |
| `probe_file()` | Probes real files (or returns error for missing files) |
| `decode()` | Decodes real packets (or returns error for missing decoder) |
| Unit tests | Test real FFmpeg behavior |
| Integration tests | Process real media files |

## Environment Variables

| Variable | Purpose | Default |
|----------|---------|---------|
| `KIVO_SAMPLE_H264_AAC_MP4` | Path to H.264+AAC MP4 sample | (none) |

## Expected Test Output

### Unit Tests (33 total)
```
real_probe_runtime_test:
  PASS: is_ffmpeg_available (deterministic)
  PASS: supported_container_formats (21 formats)
  PASS: supported_video_codecs (12 codecs)
  PASS: supported_audio_codecs (12 codecs)
  PASS: probe_file_nonexistent
  PASS: probe_file_empty_path
  PASS: probe_stream_closed
  PASS: probe_result_structure
  PASS: stream_info_structure
  PASS: probe_file_sample_media (skip)
ALL 10 TESTS PASSED

real_demux_runtime_test:
  PASS: default_construction
  PASS: open_nonexistent
  PASS: read_packet_not_open
  PASS: close_when_not_open
  PASS: move_construction
  PASS: move_assignment
  PASS: move_self_assignment
  PASS: open_close_lifecycle
  PASS: seek_to_time_not_open
  PASS: inspector_hint
  PASS: open_sample_media (skip)
ALL 11 TESTS PASSED

real_software_decode_runtime_test:
  PASS: default_construction
  PASS: open_nonexistent_codec
  PASS: decode_not_open
  PASS: flush_not_open
  PASS: close_when_not_open
  PASS: move_construction
  PASS: move_assignment
  PASS: open_h264
  PASS: open_aac
  PASS: decode_result_structure
  PASS: kivo_packet_structure
  PASS: open_close_lifecycle
ALL 12 TESTS PASSED
```

### Integration Tests (12 total)
```
local_file_byte_stream_integration_test:
  PASS: test_file_not_found_error
  PASS: test_open_and_read_real_file (1234567 bytes)
  PASS: test_read_beyond_eof
  PASS: test_reopen_same_file
ALL 4 INTEGRATION TESTS PASSED

real_probe_integration_test:
  PASS: test_probe_real_mp4 (mp4, 1920x1080 h264/aac, 120.5s)
  PASS: test_probe_via_byte_stream (stream_probe_not_supported)
  PASS: test_probe_non_media_file (open_failed)
  PASS: test_supported_format_lists (21 containers, 12 video, 12 audio)
ALL 4 INTEGRATION TESTS PASSED

real_demux_decode_integration_test:
  PASS: test_demux_decode_first_frames (50 packets read from mp4)
  PASS: test_open_decoder_with_real_codec (h264)
  PASS: test_demux_seek (seek_ok)
  PASS: test_decoder_flush
ALL 4 INTEGRATION TESTS PASSED
```

## Troubleshooting

### Tests Show "SKIP"
**Cause**: Environment variable not set or FFmpeg not enabled.

**Solution**:
```powershell
# Set sample file
$env:KIVO_SAMPLE_H264_AAC_MP4 = "C:\path\to\sample.mp4"

# Rebuild with FFmpeg
cmake --preset ninja-debug-ffmpeg
cmake --build --preset ninja-debug-ffmpeg
```

### Tests Show "ffmpeg_not_available"
**Cause**: FFmpeg compile definition not set.

**Solution**: Use `ninja-debug-ffmpeg` preset or pass `-DKIVO_ENABLE_FFMPEG=ON`.

### Linker Errors
**Cause**: FFmpeg libraries not found.

**Solution**: Install FFmpeg via vcpkg:
```powershell
vcpkg install ffmpeg:x64-windows
```

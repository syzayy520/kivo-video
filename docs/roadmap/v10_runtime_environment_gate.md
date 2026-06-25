# V10 Runtime Environment Gate

Created: 2026-06-25
Baseline: a44fae9 (P2-REAL-001 PASS)

This document verifies the runtime environment for V10 local real playback closure.

---

## 1. Operating System

- **OS**: Windows (win32)
- **Shell**: PowerShell (Core)
- **Architecture**: x64

---

## 2. Compiler

- **Compiler**: Visual Studio 18 2026 (MSVC)
- **vcvarsall.bat**: `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat`
- **C++ Standard**: C++20
- **Build Type**: Debug

---

## 3. CMake Presets

- **FFmpeg ON preset**: `vs-debug-ffmpeg`
  - Generator: Visual Studio 18 2026
  - Architecture: x64
  - KIVO_ENABLE_FFMPEG: ON
  - KIVO_ENABLE_REAL_MEDIA_TESTS: ON
  - FFMPEG_ROOT: `C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1`
- **FFmpeg OFF preset**: `vs-debug`
  - KIVO_ENABLE_FFMPEG: OFF
  - KIVO_ENABLE_REAL_MEDIA_TESTS: OFF

---

## 4. FFmpeg Libraries

- **FFmpeg SDK Path**: `C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1`
- **Libraries**: avcodec, avformat, avutil, swresample
- **Status**: Verified in P2-REAL-001 (probe/demux/decode working)

---

## 5. D3D11 Hardware / WARP / DXGI Adapter

- **D3D11**: Available (Windows default)
- **WARP**: Available as fallback for headless/CI
- **DXGI**: Available for adapter enumeration
- **COM Apartment**: MTA (Multi-Threaded Apartment)

**Proof**: D3D11 device creation tested in P2-REAL-001 integration test.

---

## 6. WASAPI Shared Endpoint / Default Render Endpoint

- **WASAPI**: Available (Windows default)
- **Shared Mode**: Supported for low-latency audio
- **Default Render Endpoint**: System default audio device
- **COM Apartment**: MTA (Multi-Threaded Apartment)

**Proof**: WASAPI shared mode tested in P2-REAL-001 integration test.

---

## 7. Sample Presence / Duration

- **Sample File**: `tests/real/test_sample.mp4`
- **Format**: H.264 video, AAC audio
- **Duration**: 1 second (short sample for unit tests)
- **Video**: 320x240, yuv420p
- **Audio**: 44100Hz, 1ch, fltp

**Note**: For 30s micro soak (V10-021), a longer sample will be needed.

---

## 8. Artifact Directory

- **Evidence Directory**: `docs/v10_evidence/`
- **Status**: Not yet created (will be created during evidence generation)
- **Schema**: Will be defined in V10-017

---

## 9. Audio Session Mute Support

- **Mute Support**: Available via WASAPI volume control
- **Implementation**: Through `IAudioSessionControl` interface
- **Status**: Not yet implemented (P3 scope)

---

## 10. COM Apartment Model

- **D3D11**: MTA (Multi-Threaded Apartment)
- **WASAPI**: MTA (Multi-Threaded Apartment)
- **FFmpeg**: No COM dependency
- **Thread Safety**: All COM objects accessed from same apartment

---

## 11. Runtime Environment Risk Assessment

| Risk | Status | Mitigation |
|------|--------|-----------|
| D3D11 unavailable (CI/headless) | LOW | WARP fallback available |
| WASAPI unavailable (CI/headless) | LOW | Loopback/silence device fallback |
| FFmpeg SDK missing | BLOCKED | Must exist at FFMPEG_ROOT |
| Sample file missing | BLOCKED | Must exist at tests/real/test_sample.mp4 |
| COM apartment issues | LOW | MTA used for all COM objects |

---

## 12. Validation Commands

```bash
# Verify FFmpeg SDK exists
dir "C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1"

# Verify sample file exists
dir "tests\real\test_sample.mp4"

# Verify CMake preset works
cmake --preset vs-debug-ffmpeg

# Verify build works
cmake --build --preset vs-debug-ffmpeg

# Verify tests pass
ctest --preset vs-debug-ffmpeg --output-on-failure
```

---

## 13. Conclusion

Runtime environment is READY for V10 local real playback closure.

**Status**: PASS

**Next**: Proceed to V10-007 Local Source Identity / Local Byte Stream Proof.
# KIVO-VIDEO-P6D-PLANNING-DRAFT-001

**Document Type**: planning-draft (design-only, ZERO implementation until approved)
**Source**: P6 Audio Media Plane Design Lock Candidate V2.0 (§5, §13, §19, §25, §33)
**Predecessors**: P6A Contract Layer (CLOSED), P6B Fake Runtime Bridge (CLOSED), P6C FFmpeg Decode Backend (CLOSED)
**Stage**: P6D (WASAPI Output Backend — real IAudioClient, isolated)
**Status**: CLOSED (closure report: docs/roadmap/p6d_closure_report.md)
**Generated**: 2026-06-29
**Rule**: IRON GATE — 禁止此 Draft APPROVED 前任何 .hpp / .cpp / CMake 创建

---

## WORKSPACE LOCK

| # | Rule |
|---|------|
| W1 | 唯一工作目录：`C:\kivo video` |
| W2 | 禁止新建 worktree |
| W3 | 禁止 clone 仓库到任何其他位置 |
| W4 | 禁止在 `C:\kivo video` 之外的任何目录读写 P6D 相关文件 |
| W5 | 所有 commit/push 必须在 `C:\kivo video` 本仓库 `kivo-video-p5a-contract-layer-001` 分支完成 |
| W6 | 禁止外部目录作为 "临时" / "备份" / "参考" |

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

## 0. P6D SCOPE DEFINITION

### 0.1 一句话定义

P6D = **WASAPI Output Backend** — 真实的 Windows WASAPI 音频输出后端，隔离在 `output/wasapi/` 私有目录中，消费 P6A output 合同 + P6B fake render 接口模式，不涉及 resampler 或 DSP。

### 0.2 P6D 允许做

| # | Capability |
|---|-----------|
| 1 | Real WASAPI device enumeration (IMMDeviceEnumerator) |
| 2 | Real WASAPI audio client (IAudioClient) — open, start, stop, reset |
| 3 | Real WASAPI render client (IAudioRenderClient) — GetBuffer/ReleaseBuffer |
| 4 | Real WASAPI session control (IAudioSessionControl) — session events |
| 5 | Real WASAPI endpoint volume (IAudioEndpointVolume) — volume/mute |
| 6 | Real WASAPI audio clock (IAudioClock) — position feedback |
| 7 | Device notification (IMMNotificationClient) — device lost/hotplug |
| 8 | COM initialization (CoInitializeEx) for device notification thread |
| 9 | 12-state output backend lifecycle (NotCreated→Created→...→Closed) |
| 10 | Event-driven render loop (WAIT_OBJECT_0 on buffer handle) |
| 11 | Output evidence real generation (9 kinds) |
| 12 | Device lost detection + recovery transition |

### 0.3 P6D 禁止做

| # | Forbidden |
|---|----------|
| 1 | ❌ Real resampler (libswresample, P6E scope) |
| 2 | ❌ Real DSP chain (dither/quantize/limiter, P6E scope) |
| 3 | ❌ Real FFmpeg decode (P6C scope) |
| 4 | ❌ Real passthrough parse (P6E scope) |
| 5 | ❌ Real clock/sync runtime (P6E scope) |
| 6 | ❌ Public header 中暴露 WASAPI 类型 (IAudioClient/IAudioRenderClient/IMMDevice/HRESULT/WAVEFORMATEX) |
| 7 | ❌ P6A contract header modification (298 headers FROZEN) |
| 8 | ❌ P6B runtime header modification (22 headers FROZEN) |
| 9 | ❌ P6C decode backend modification (FROZEN) |
| 10 | ❌ Qt / QML / D3D11 / DXGI |
| 11 | ❌ P4/P5/P8 file modification |
| 12 | ❌ FFmpeg calls (P6C scope) |

### 0.4 P6D Boundary (CRITICAL)

| Aspect | What it IS | What it is NOT |
|--------|-----------|----------------|
| P6D | Real WASAPI output backend | NOT P6C (FFmpeg decode) |
| P6D | Isolated in `backend/src/video/audio_plane/output/wasapi/` | NOT public include |
| P6D | Consumes P6A output contracts | NOT modifies P6A contracts |
| P6D | Follows P6B fake render interface pattern | NOT modifies P6B runtime |
| P6D | Real IAudioClient/IAudioRenderClient | NOT resampler/DSP |
| P6D | WASAPI shared/exclusive mode | NOT D3D11/FFmpeg |
| P6D | COM init for notification thread | NOT UI thread COM |

---

## 1. PROPOSED NATURAL FAMILY TREE

### 1.1 Directory Structure

P6D creates **private backend** files only. No public headers. All WASAPI types are isolated in `backend/src/video/audio_plane/output/wasapi/` and `backend/include_private/video/audio_plane/output/wasapi/`.

```
backend/
│
├── src/video/audio_plane/output/wasapi/          # P6D WASAPI output backend (NEW)
│   │
│   ├── internal/                                  # WASAPI internal headers
│   │   ├── wasapi_com_initializer_guard.hpp        # RAII guard for CoInitializeEx/CoUninitialize
│   │   ├── wasapi_device_enumerator_guard.hpp      # RAII guard for IMMDeviceEnumerator
│   │   ├── wasapi_device_guard.hpp                 # RAII guard for IMMDevice
│   │   ├── wasapi_audio_client_guard.hpp           # RAII guard for IAudioClient
│   │   ├── wasapi_render_client_guard.hpp          # RAII guard for IAudioRenderClient
│   │   ├── wasapi_session_control_guard.hpp        # RAII guard for IAudioSessionControl
│   │   ├── wasapi_endpoint_volume_guard.hpp        # RAII guard for IAudioEndpointVolume
│   │   ├── wasapi_audio_clock_guard.hpp            # RAII guard for IAudioClock
│   │   └── wasapi_format_helper.hpp                # WAVEFORMATEX construction/parsing
│   │
│   ├── wasapi_output_backend.cpp                   # WasapiOutputBackend implementation
│   ├── wasapi_output_lifecycle.cpp                 # Lifecycle controller (real transitions)
│   ├── wasapi_device_enumerator.cpp                # Device enumeration + notification
│   ├── wasapi_render_loop.cpp                      # Event-driven render loop
│   └── wasapi_output_evidence_emitter.cpp          # Output evidence generation
│
├── include_private/video/audio_plane/output/wasapi/  # P6D private interface (NEW)
│   │
│   ├── wasapi_output_backend.hpp                    # WasapiOutputBackend class declaration
│   ├── wasapi_output_lifecycle.hpp                  # WasapiOutputLifecycleController class
│   ├── wasapi_device_enumerator.hpp                 # Device enumerator + notification
│   ├── wasapi_render_loop.hpp                       # Render loop (event-driven)
│   └── wasapi_output_evidence_emitter.hpp           # Evidence emitter interface
│
└── tests/video/audio_plane/p6d_wasapi_output_tests/  # P6D tests (NEW)
    ├── wasapi_output_backend_tests.cpp              # D1-D8: backend lifecycle
    ├── wasapi_render_loop_tests.cpp                 # D9-D12: render loop
    └── wasapi_output_evidence_tests.cpp             # D13-D15: evidence emission
```

### 1.2 Statistics

| Category | Count |
|----------|-------|
| Backend source directories | 2 (src + include_private) |
| Internal headers | 9 |
| Private interface headers | 5 |
| Backend source files (.cpp) | 5 |
| Test files | 3 |
| CMake targets | 1 new (kivo_p6d_wasapi_output_tests) |
| Namespace | kivo::video::audio_plane::backend::output::wasapi |

### 1.3 No Public Headers (CRITICAL — WASAPI type isolation)

P6D creates **ZERO** public headers. All WASAPI types are private:
- `IAudioClient`, `IAudioRenderClient`, `IMMDevice`, `IMMDeviceEnumerator`, `IAudioSessionControl`, `IAudioEndpointVolume`, `IAudioClock`, `HRESULT`, `WAVEFORMATEX`, `WAVEFORMATEXTENSIBLE`, `GUID`, `CoInitializeEx` exist ONLY in `backend/src/.../internal/` and `backend/include_private/.../`
- Public include graph must NOT depend on `backend/include_private/`
- Public P6A contracts (`AudioOutputBackendState`, `AudioOutputOpenRequest`, etc.) are consumed, not modified

---

## 2. P6D SCOPE: 15-POINT PLANNING INVENTORY

### 2.1 COM Initializer Guard (D1)
- `wasapi_com_initializer_guard.hpp`: RAII wrapper for CoInitializeEx/CoUninitialize, per-thread, Apartment or MTA

### 2.2 Device Enumerator Guard (D2)
- `wasapi_device_enumerator_guard.hpp`: RAII for IMMDeviceEnumerator (CoCreateInstance), EnumAudioEndpoints, GetDefaultEndpoint

### 2.3 Device Guard (D3)
- `wasapi_device_guard.hpp`: RAII for IMMDevice, Activate IAudioClient/IAudioEndpointVolume

### 2.4 Audio Client Guard (D4)
- `wasapi_audio_client_guard.hpp`: RAII for IAudioClient, Initialize/Start/Stop/Reset/GetBufferDuration

### 2.5 Render Client Guard (D5)
- `wasapi_render_client_guard.hpp`: RAII for IAudioRenderClient, GetBuffer/ReleaseBuffer

### 2.6 Session Control Guard (D6)
- `wasapi_session_control_guard.hpp`: RAII for IAudioSessionControl, session events

### 2.7 Endpoint Volume Guard (D7)
- `wasapi_endpoint_volume_guard.hpp`: RAII for IAudioEndpointVolume, volume/mute/query

### 2.8 Audio Clock Guard (D8)
- `wasapi_audio_clock_guard.hpp`: RAII for IAudioClock, position/frequency

### 2.9 Format Helper (D9)
- `wasapi_format_helper.hpp`: WAVEFORMATEX/WAVEFORMATEXTENSIBLE construction from P6 AudioFormatDescriptor, validation

### 2.10 WasapiOutputBackend (D10)
- `wasapi_output_backend.hpp/.cpp`: init/open/start/stop/pause/flush/drain/close/transition_to, 12-state lifecycle

### 2.11 WasapiOutputLifecycleController (D11)
- `wasapi_output_lifecycle.hpp/.cpp`: transition validation, device lost detection, recovery

### 2.12 Device Enumerator + Notification (D12)
- `wasapi_device_enumerator.hpp/.cpp`: IMMNotificationClient, device added/removed/changed

### 2.13 Render Loop (D13)
- `wasapi_render_loop.hpp/.cpp`: event-driven loop, WAIT_OBJECT_0, fill buffer from SPSC queue

### 2.14 Evidence Emitter (D14)
- `wasapi_output_evidence_emitter.hpp/.cpp`: 9 kinds (OpenOk/OpenFailed/StartOk/StartFailed/StopOk/DeviceLost/Recovered/BufferUnderrun/Closed)

### 2.15 P6A State Model Mapping (D15)
- All P6D lifecycle events map to P6A AudioOutputBackendState (12 states), no new states

---

## 3. P6D EXIT CRITERIA (15 ITEMS)

| # | Criterion | Coverage | Status |
|---|-----------|----------|--------|
| E1 | COM initializer RAII guard | §2.1 (D1) | ✅ DESIGNED |
| E2 | Device enumerator RAII guard | §2.2 (D2) | ✅ DESIGNED |
| E3 | Device RAII guard | §2.3 (D3) | ✅ DESIGNED |
| E4 | Audio client RAII guard | §2.4 (D4) | ✅ DESIGNED |
| E5 | Render client RAII guard | §2.5 (D5) | ✅ DESIGNED |
| E6 | Session control RAII guard | §2.6 (D6) | ✅ DESIGNED |
| E7 | Endpoint volume RAII guard | §2.7 (D7) | ✅ DESIGNED |
| E8 | Audio clock RAII guard | §2.8 (D8) | ✅ DESIGNED |
| E9 | Format helper (WAVEFORMATEX) | §2.9 (D9) | ✅ DESIGNED |
| E10 | WasapiOutputBackend (init/open/start/stop/pause/flush/drain/close) | §2.10 (D10) | ✅ DESIGNED |
| E11 | Lifecycle controller (12-state + device lost + recovery) | §2.11 (D11) | ✅ DESIGNED |
| E12 | Device enumerator + notification (IMMNotificationClient) | §2.12 (D12) | ✅ DESIGNED |
| E13 | Render loop (event-driven, WAIT_OBJECT_0) | §2.13 (D13) | ✅ DESIGNED |
| E14 | Output evidence emitter (9 kinds) | §2.14 (D14) | ✅ DESIGNED |
| E15 | P6A state model mapping (no new states) | §2.15 (D15) | ✅ DESIGNED |

**Coverage: 15/15 DESIGNED (0 NOT COVERED)**

---

## 4. P6D FORBIDDEN FILES

| # | Forbidden Item |
|---|---------------|
| F-01 | ❌ Any modification to P6A/P6B/P6C files (FROZEN) |
| F-02 | ❌ Any public header (P6D = private backend only) |
| F-03 | ❌ Any real resampler/DSP (P6E scope) |
| F-04 | ❌ Any real FFmpeg call (P6C scope) |
| F-05 | ❌ Public exposure of WASAPI types |
| F-06 | ❌ Qt / QML / D3D11 / DXGI |
| F-07 | ❌ P4/P5/P8 file modification |
| F-08 | ❌ Any file outside allowed directories |
| **Allow** | ✅ Private backend .cpp/.hpp in output/wasapi/ |

---

## 5. CMake TARGET STRUCTURE

### 5.1 KIVO_ENABLE_WASAPI Default Behavior

| Setting | Default | Behavior |
|---------|---------|----------|
| `KIVO_ENABLE_WASAPI` | **OFF** | P6D target NOT created, full repo builds normally |
| `KIVO_ENABLE_WASAPI=ON` + Windows | ON | P6D target created, links ole32/mmdevapi |
| `KIVO_ENABLE_WASAPI=ON` + non-Windows | WARNING + skip | P6D target NOT created |

### 5.2 CMake Snippet

```cmake
if(KIVO_ENABLE_WASAPI AND WIN32)
    add_executable(kivo_p6d_wasapi_output_tests ...)
    target_link_libraries(kivo_p6d_wasapi_output_tests PRIVATE
      kivo_audio_plane_contracts ole32 mmdevapi)
    target_compile_options(... PRIVATE /utf-8)
endif()
```

---

## 6. WHETHER READY FOR P6D IMPLEMENTATION

| Check | Result |
|-------|--------|
| P6A contract layer closed? | ✅ |
| P6B fake runtime closed? | ✅ |
| P6C FFmpeg decode closed? | ✅ |
| V2.0 design lock confirmed? | ✅ |
| All 15 planning items covered? | ✅ 15/15 DESIGNED |
| No forbidden scope creep? | ✅ NO resampler/DSP/FFmpeg |

### READY FOR P6D IMPLEMENTATION: YES ⚠️ CONDITIONAL

**Conditions**:
1. User must explicitly approve this Planning Draft
2. P6A/P6B/P6C files must remain FROZEN
3. Windows platform required (WASAPI)

---

**END OF P6D PLANNING DRAFT 001**
**Status**: DRAFT_FOR_REVIEW — awaiting user confirmation

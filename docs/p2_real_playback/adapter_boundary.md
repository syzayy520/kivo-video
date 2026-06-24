# P2-002 Adapter Boundary Hardening

Updated: 2026-06-25

## Goal

Harden the adapter boundary rules to ensure platform-specific types (FFmpeg, D3D11, DXGI, WASAPI, Win32, COM) never leak into core modules. This document defines the complete set of forbidden patterns and the enforcement mechanism.

## Adapter Boundary Principle

Core modules (everything under `include/kivo/cinema_engine/` except `adapter_boundary/`) must never include or reference platform-specific types. Platform types are confined to adapter modules that sit behind the `AdapterBoundaryContract`.

## Forbidden Patterns

### Windows Platform Types

| Pattern | Description |
|---------|-------------|
| `#include <windows.h>` | Win32 base header |
| `#include <d3d11.h>` | Direct3D 11 |
| `#include <dxgi.h>` | DirectX Graphics Infrastructure |
| `#include <audioclient.h>` | WASAPI audio client |
| `#include <mmdeviceapi.h>` | Multimedia device API |
| `HWND` | Window handle |
| `HANDLE` | Kernel object handle |
| `HRESULT` | COM return type |
| `GUID` / `IID` / `CLSID` | COM identifiers |

### FFmpeg Types

| Pattern | Description |
|---------|-------------|
| `AVFormatContext` | FFmpeg format context |
| `AVCodecContext` | FFmpeg codec context |
| `AVFrame` | FFmpeg decoded frame |
| `AVPacket` | FFmpeg encoded packet |
| `AVBufferRef` | FFmpeg buffer reference |
| `AVStream` | FFmpeg stream |
| `avformat_*` | FFmpeg format functions |
| `avcodec_*` | FFmpeg codec functions |
| `av_frame_*` | FFmpeg frame functions |
| `av_packet_*` | FFmpeg packet functions |

### Platform-Specific Struct Names in Core

Core contract struct names must be platform-neutral:

| ❌ Forbidden | ✅ Correct |
|-------------|-----------|
| `D3D11Presenter` | `PresenterRuntimeContract` |
| `WasapiEndpoint` | `AudioEndpointRuntimeContract` |
| `FFmpegDemuxRuntime` | `DemuxRuntime` |
| `FFmpegDecodeRuntime` | `DecodeRuntime` |
| `Win32FileIdentity` | `LocalFileIdentity` |

## Enforcement

### Gate: `check_adapter_boundary_directory_rules()`

This gate scans all `.hpp` and `.cpp` files in core directories for forbidden patterns:

1. **Core include directories**: All directories under `include/kivo/cinema_engine/` except `adapter_boundary/`
2. **Core source directories**: All directories under `src/` except `adapter_boundary/`
3. **Core test directories**: All directories under `tests/` except `adapter_boundary/`

### Gate: `check_p2_no_adapter_leak_in_new_headers()`

This gate specifically checks P2-001A and P2-001C headers for forbidden patterns.

### Gate: `check_forbidden_includes()`

This gate checks all `.hpp` and `.cpp` files for forbidden `#include` directives and type references.

## Adapter Module Exceptions

The following directories are allowed to contain platform-specific types:

- `backend/include/kivo/cinema_engine/adapter_boundary/` — Adapter boundary contracts
- `backend/src/adapter_boundary/` — Adapter boundary implementations
- `backend/tests/adapter_boundary/` — Adapter boundary tests

Future adapter modules (e.g., `backend/src/ffmpeg_adapter/`, `backend/src/d3d11_adapter/`) will also be allowed to contain platform types.

## Verification

P2-002 is verified when:
1. All existing P1 checks still pass
2. All P2 headers pass the adapter boundary check
3. No platform-specific types leak into core modules
4. `python backend/tools/governance/verify_p1_gates.py` outputs "PASS: P0/P1/P2 governance gates"

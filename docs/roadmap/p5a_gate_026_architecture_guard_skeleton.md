# GATE-026: Architecture Guard Target Graph Skeleton

**Task ID**: KIVO-VIDEO-P5-GATE-026-ARCH-SKELETON-001
**Document Type**: design-skeleton (NO code yet)
**Gate Stage**: P5A (initial definition)
**Status**: DRAFT

---

## CMake Target Graph (Design Only)

```
kivo_video_plane_contracts (INTERFACE)
├── include/kivo/video_plane/  (109 headers)
├── LINK: NOTHING
├── DEPENDS: kivo_core_contract (P0 types only)
└── NAMESPACE: kivo::video_plane

kivo_video_plane_core (STATIC)           ← P5C+
├── src/video_plane/core/
├── LINK: kivo_video_plane_contracts
├── MAY NOT LINK: FFmpeg, D3D11, DXGI, Qt, WASAPI
└── NAMESPACE: kivo::video_plane::core

kivo_ffmpeg_demux (STATIC)               ← P5C
├── src/video_plane/ffmpeg_demux/
├── LINK: kivo_video_plane_core, FFmpeg::avformat
├── ISOLATED: ffmpeg_demux/ directory only
└── NAMESPACE: kivo::video_plane::ffmpeg_demux

kivo_ffmpeg_decode (STATIC)              ← P5D+
├── src/video_plane/ffmpeg_decode/
├── LINK: kivo_video_plane_core, FFmpeg::avcodec
├── ISOLATED: ffmpeg_decode/ directory only
└── NAMESPACE: kivo::video_plane::ffmpeg_decode

kivo_d3d11_gpu (STATIC)                  ← P5F+
├── src/video_plane/d3d11_gpu/
├── LINK: kivo_video_plane_core, D3D11, DXGI
├── ISOLATED: d3d11_gpu/ directory only
└── NAMESPACE: kivo::video_plane::d3d11_gpu

kivo_dxgi_swapchain (STATIC)             ← P5F+
├── src/video_plane/dxgi_swapchain/
├── LINK: kivo_video_plane_core, DXGI
├── ISOLATED: dxgi_swapchain/ directory only
└── NAMESPACE: kivo::video_plane::dxgi_swapchain

kivo_video_plane_tests (EXECUTABLE)      ← P5A→P5H
├── tests/video_plane/
├── LINK: kivo_video_plane_contracts
├── CI: fake backends only
├── Nightly: stress tests
├── Lab: real D3D11/VRR/HDR
└── RC: full compatibility matrix

kivo_video_plane_future_backends (INTERFACE) ← future, default OFF
├── NOT in default include path
├── NOT in baseline binary
└── NAMESPACE: kivo::video_plane::future
```

---

## Link Rules (Design)

| # | Rule | Check Method |
|---|------|-------------|
| 1 | `kivo_video_plane_contracts` links NO runtime libraries | CMake dependency graph |
| 2 | `kivo_video_plane_contracts` has NO `target_link_libraries` to FFmpeg/D3D11/DXGI/Qt/WASAPI | CMake verify script |
| 3 | `kivo_ffmpeg_demux` MAY link FFmpeg but ONLY in `ffmpeg_demux/` directory | Path isolation check |
| 4 | `kivo_d3d11_gpu` MAY link D3D11 but ONLY in `d3d11_gpu/` directory | Path isolation check |
| 5 | Public headers contain NO raw third-party types (`AVFormatContext`, `ID3D11Device`, etc.) | Forbidden token scan |
| 6 | Binary exports contain NO forbidden symbols | `dumpbin /exports` scan |
| 7 | Future backends NOT in default include path | CMake `target_include_directories` check |
| 8 | Future backends NOT linked into baseline binary | CMake dependency graph |
| 9 | Core target exposes NO FFmpeg/D3D11 public ABI | Header scan |
| 10 | Root CMakeLists.txt only adds `video_plane/` subdirectory when P5 enabled | CMake `option()` guard |

---

## Symbol Checker Definitions (Design)

```yaml
PublicExportSymbolScan:
  target: kivo_video_plane_contracts
  allowed: kivo::video_plane::*
  forbidden: 
    - av_*           # FFmpeg
    - ID3D11*        # D3D11
    - IDXGI*         # DXGI
    - Q*             # Qt
    - IAudioClient*  # WASAPI

InternalSymbolScan:
  target: kivo_video_plane_core
  forbidden:
    - ID3D11Device::CreateTexture2D   # No direct D3D11 in core
    - avformat_open_input             # No direct FFmpeg in core

DependencyGraphScan:
  target: all
  rules:
    - contracts → (nothing)
    - core → contracts
    - ffmpeg_demux → core + FFmpeg::avformat
    - ffmpeg_decode → core + FFmpeg::avcodec
    - d3d11_gpu → core + D3D11 + DXGI
    - dxgi_swapchain → core + DXGI
    - tests → contracts (+ fake in CI, + real in Lab)
    - future → contracts (NOT in baseline)
```

---

## Root CMakeLists.txt Integration (Design)

```cmake
# In root CMakeLists.txt or backend/CMakeLists.txt:
option(KIVO_ENABLE_P5_VIDEO_PLANE "Enable P5 Video Media Plane" OFF)

if(KIVO_ENABLE_P5_VIDEO_PLANE)
    add_subdirectory(video_plane)
endif()
```

---

## Gate Verification Script (Design)

Script: `backend/tools/governance/check_p5_architecture_guard.ps1`

Checks:
1. CMake target existence (7 targets)
2. Link dependency validity (10 rules)
3. Forbidden symbol scan on contracts target
4. Public header third-party type scan
5. Path isolation verification
6. Future backend isolation verification

---

**END OF GATE-026 SKELETON**
**Status**: DRAFT — no CMakeLists.txt created until P5A Planning Draft approved

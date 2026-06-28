# KIVO-VIDEO-P6E-PLANNING-DRAFT-001

**Document Type**: planning-draft
**Stage**: P6E (Resampler / DSP / Clock / Sync Runtime — real implementations)
**Status**: PARTIALLY CLOSED (audit REV 001 — 6 gaps acknowledged, see p6e_closure_report.md)
**Generated**: 2026-06-29

---

## 0. P6E SCOPE DEFINITION

P6E = **Resampler / DSP / Clock / Sync Runtime** — 真实的音频处理 runtime，消费 P6A 合同 + P6B fake 接口模式。

### 0.1 P6E 允许做

| # | Capability |
|---|-----------|
| 1 | Real sample rate conversion (linear interpolation resampler) |
| 2 | Real DSP chain (dither / quantize / noise shaping) |
| 3 | Real audio clock (position tracking, drift detection) |
| 4 | Real A/V sync decision (clock arbitration) |
| 5 | Real lock-free SPSC render queue (atomic-based) |
| 6 | Real passthrough parse (IEC61937 burst framing detection) |

### 0.2 P6E 禁止做

| # | Forbidden |
|---|----------|
| 1 | ❌ Real WASAPI calls (P6D scope) |
| 2 | ❌ Real FFmpeg decode (P6C scope) |
| 3 | ❌ Public header modification (P6A FROZEN) |
| 4 | ❌ P6B/P6C/P6D file modification (FROZEN) |
| 5 | ❌ Qt / D3D11 / DXGI |
| 6 | ❌ libswresample (P6E uses self-implemented resampler, NOT FFmpeg swresample) |

---

## 1. PROPOSED NATURAL FAMILY TREE

```
backend/
├── src/video/audio_plane/runtime/resampler/     # P6E real resampler
│   ├── linear_resampler.cpp                      # Linear interpolation resampler
│   └── resampler_factory.cpp                     # Resampler factory
├── src/video/audio_plane/runtime/dsp/            # P6E real DSP chain
│   ├── dither_processor.cpp                      # Dither processor
│   ├── quantize_processor.cpp                    # Quantization processor
│   └── noise_shaping_processor.cpp               # Noise shaping processor
├── src/video/audio_plane/runtime/clock/          # P6E real clock
│   ├── audio_clock.cpp                           # Audio clock implementation
│   └── drift_corrector.cpp                       # Drift correction
├── src/video/audio_plane/runtime/sync/           # P6E real A/V sync
│   └── av_sync_controller.cpp                    # A/V sync controller
├── src/video/audio_plane/runtime/spsc/           # P6E real lock-free SPSC
│   └── lock_free_spsc_queue.cpp                  # Lock-free SPSC render queue
├── src/video/audio_plane/runtime/passthrough/    # P6E real passthrough parse
│   └── iec61937_parser.cpp                       # IEC61937 burst framing parser
├── include_private/video/audio_plane/runtime/resampler/
│   ├── linear_resampler.hpp
│   └── resampler_factory.hpp
├── include_private/video/audio_plane/runtime/dsp/
│   ├── dither_processor.hpp
│   ├── quantize_processor.hpp
│   └── noise_shaping_processor.hpp
├── include_private/video/audio_plane/runtime/clock/
│   ├── audio_clock.hpp
│   └── drift_corrector.hpp
├── include_private/video/audio_plane/runtime/sync/
│   └── av_sync_controller.hpp
├── include_private/video/audio_plane/runtime/spsc/
│   └── lock_free_spsc_queue.hpp
├── include_private/video/audio_plane/runtime/passthrough/
│   └── iec61937_parser.hpp
└── tests/video/audio_plane/p6e_runtime_tests/
    ├── resampler_tests.cpp
    ├── dsp_tests.cpp
    ├── clock_sync_tests.cpp
    ├── spsc_tests.cpp
    └── passthrough_tests.cpp
```

---

## 2. P6E EXIT CRITERIA (12 ITEMS)

| # | Criterion | Status |
|---|-----------|--------|
| E1 | Linear resampler (sample rate conversion) | ✅ DESIGNED |
| E2 | Resampler factory | ✅ DESIGNED |
| E3 | Dither processor | ✅ DESIGNED |
| E4 | Quantize processor | ✅ DESIGNED |
| E5 | Noise shaping processor | ✅ DESIGNED |
| E6 | Audio clock (position tracking) | ✅ DESIGNED |
| E7 | Drift corrector | ✅ DESIGNED |
| E8 | A/V sync controller | ✅ DESIGNED |
| E9 | Lock-free SPSC render queue | ✅ DESIGNED |
| E10 | IEC61937 passthrough parser | ✅ DESIGNED |
| E11 | P6A state model mapping | ✅ DESIGNED |
| E12 | No libswresample dependency | ✅ DESIGNED |

**Coverage: 12/12 DESIGNED**

---

**END OF P6E PLANNING DRAFT 001**

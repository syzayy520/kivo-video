# P6G-039: Negative Fixtures — Forbidden Pattern Coverage

## Overview

P6G-039 requires compile-fail tests for 8 forbidden patterns (§42, §44).
This directory contains 2 compile-fail fixtures. The remaining 6 forbidden
patterns are enforced by the architecture guard scanner (CTest #181) and
positive contract tests (CTest #180).

## Compile-Fail Fixtures (WILL_FAIL=TRUE)

| # | Fixture | Forbidden Pattern | Enforcement |
|---|---------|-------------------|-------------|
| 1 | `neg_spsc_bool_conversion.cpp` | bool return from SPSC queue (§15) | enum class prevents implicit bool conversion → compile fail |
| 2 | `neg_span_in_access_unit.cpp` | span stored in EncodedAccessUnit (§8, §12) | payload_handle_id is uint64_t, span assignment → compile fail |

## Guard Scanner Coverage (CTest #181)

The following forbidden patterns are enforced by `p6_audio_arch_guard.py`
mode `PublicHeadersStrict` (CTest #181, PASS):

| # | Forbidden Pattern | Guard Rule |
|---|-------------------|------------|
| 3 | public header with WASAPI type | FORBIDDEN_TOKENS: IAudioClient, IAudioRenderClient, etc. |
| 4 | public header with FFmpeg type | FORBIDDEN_TOKENS: AVFrame, AVPacket, AVCodecContext, etc. |
| 5 | public header with Qt type | FORBIDDEN_TOKENS: QString, QByteArray, QObject, etc. |

## Positive Test Coverage (CTest #180)

The following forbidden patterns are verified by positive assertions in
`p6a_contract_tests.cpp` (CTest #180, PASS):

| # | Forbidden Pattern | Test Line | Verification |
|---|-------------------|-----------|--------------|
| 6 | missing TimedOut state | line 137 | `lc.state = S::TimedOut; lc.is_timed_out()` confirms TimedOut enum value exists |
| 7 | bit-perfect claim with Unknown state | line 147 | `gate.all_conditions_met()` returns false when conditions are false (Unknown blocks claim) |
| 8 | dither after quantization | field check | `AudioDitherPolicy.applied_before_quantization{true}` documents dither-before-quantize order |

## CMake Integration

Negative fixtures are registered via `add_subdirectory(tests/video/audio_plane/negative)`
in `backend/CMakeLists.txt`. Each fixture uses `set_tests_properties(... WILL_FAIL TRUE)`
so CTest expects compilation to fail.

## Running

```bash
ctest --preset vs-debug -R "p6_neg_"
```

Expected: 2/2 PASS (each "pass" = compile failed as expected).

# P4 Negative Compile Test Plan

Gate: P4G-008 (partial), P4G-062

## Status: PLAN + SKELETON (not yet wired to CMake as compile-fail targets)

Negative compile tests verify that the type system rejects illegal conversions at compile time.
Each test must FAIL to compile. If any compiles successfully, the type system is broken.

## Required Negative Compile Tests

| # | Test | Expected Compile Error |
|---|------|----------------------|
| 1 | Milliseconds cannot pass to Bytes | type mismatch |
| 2 | Bytes cannot pass to Count | type mismatch |
| 3 | BitsPerSecond cannot pass to Bytes | type mismatch |
| 4 | BytesPerSecond cannot pass to BitsPerSecond | type mismatch |
| 5 | PlaybackRateRatio cannot be constructed from double | no implicit double conversion |
| 6 | CommandId cannot pass to SessionId | type mismatch |
| 7 | ReaderInstanceId cannot pass to MemoryBlockId | type mismatch |

## Implementation Notes

- Each negative test is a separate .cpp file that should FAIL to compile.
- CMake should use `set_tests_properties(... WILL_FAIL TRUE)` or try_compile checks.
- Negative tests must NOT be compiled into production target.
- Current status: skeleton created, CMake wiring deferred to P4G-062 full implementation.

## P4G-008 Status

Without negative compile coverage, P4G-008 is **runtime_verified** (17 runtime tests pass), NOT full gate PASS.
Full PASS requires P4G-062 negative compile tests wired to CMake.

# P2-043 Telemetry Sampling / Hot Path Policy

## Purpose

Defines telemetry sampling policy and hot-path restrictions: bounded ring buffers prevent unbounded memory growth, and no locks are allowed on hot paths (audio/video decode, render, output).

## Scope

- Telemetry ring buffer configuration
- Sampling rate policy
- Hot-path lock prohibition
- Bounded memory guarantees

## Family Tree (diagnostic_core/)

```
diagnostic_core/
├── telemetry_ring_buffer.hpp          — Ring buffer contract (existing from P2-001C)
├── telemetry_sample_request.hpp       — Sample write request
├── telemetry_sample_result.hpp        — Sample write result
├── fake_telemetry_sampling_service.hpp — Fake service for testing
```

## Contract Types

### TelemetryRingBuffer (existing, expanded)

| Field | Type | Description |
|-------|------|-------------|
| buffer_id | string | Unique buffer identifier |
| capacity_samples | int64_t | Max samples before overwrite |
| current_samples | int64_t | Current sample count |
| sample_kind | string | Type of telemetry |
| is_bounded | bool | Whether buffer is bounded |

### TelemetrySampleRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique request identifier |
| buffer_id | string | Target ring buffer |
| sample_kind | string | Telemetry type |
| timestamp_ns | int64_t | Sample timestamp (nanoseconds) |
| value | double | Sample value |

### TelemetrySampleResult

| Field | Type | Description |
|-------|------|-------------|
| result_id | string | Unique result identifier |
| request_id | string | Reference to request |
| was_written | bool | Whether sample was written |
| was_overwritten | bool | Whether oldest sample was evicted |
| current_occupancy | int64_t | Current buffer occupancy |

### TelemetrySamplingPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| max_sample_rate_hz | int64_t | Max samples per second |
| allow_hot_path | bool | Whether this telemetry is allowed on hot path |
| ring_buffer_capacity | int64_t | Default ring buffer size |

## Tests

10 tests covering:
1. Ring buffer construction with default capacity
2. Sample request construction
3. Successful sample write result
4. Overwrite result (capacity exceeded)
5. Sampling policy construction
6. Hot-path allowed policy
7. Hot-path forbidden policy
8. Ring buffer overflow tracking
9. Bounded memory guarantee
10. Multi-kind telemetry separation

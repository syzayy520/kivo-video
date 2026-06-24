# P2-007 Timebase Normalization

Updated: 2026-06-25

## Goal

Implement timebase normalization that converts between container timebase, stream timebase, and KivoMediaTime (microseconds).

## Time Types

| Type | Description | Resolution |
|------|-------------|------------|
| ContainerTime | Raw timestamp from container (e.g., MKV, MP4) | Container timebase (e.g., 1/1000) |
| StreamTime | Timestamp in stream timebase (e.g., video 1/90000) | Stream timebase |
| KivoMediaTime | Normalized time in microseconds | 1 us |

## Conversion Formula

```
KivoMediaTime_us = ContainerTime * (container_timebase_num / container_timebase_den) * 1000000
KivoMediaTime_us = StreamTime * (stream_timebase_num / stream_timebase_den) * 1000000
```

## Implementation Plan

1. Create `FakeTimebaseService` that performs normalization
2. Tests covering conversion accuracy and edge cases
3. Design doc

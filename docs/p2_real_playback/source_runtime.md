# P2-011 Local File Source Runtime

Updated: 2026-06-25

## Goal

Implement local file source adapter that produces ByteStreamContract for local file reading.

## Source Kinds

| Kind | Description | Seekable | Readable |
|------|-------------|----------|----------|
| local_file | Direct file access | Yes | Yes |
| network_range | HTTP range request | Yes | Yes |
| cache_segment | Cached segment | Yes | Yes |

## Implementation Plan

1. Create `FakeLocalFileSourceService` that produces ByteStreamContract
2. Tests covering stream creation, field access, and source kinds
3. Design doc

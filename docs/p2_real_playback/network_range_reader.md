# P2-030 Network Range Reader

Updated: 2026-06-25

## Goal

Establish network range reader contract behind the source adapter boundary, providing byte-level range reading for remote content.

## Scope

Define contracts for network range reading: reader identity, range read requests with byte offset/length, and range read results with metadata.

## Contract Types

### NetworkRangeReader

Identity and capability declaration for a network range reader.

```cpp
struct NetworkRangeReader {
    ContractMetadata metadata;
    std::string reader_id;
    std::string adapter_id;
    std::string base_url;
    bool supports_range{true};
    int64_t max_range_size{0};  // 0 = no limit
};
```

### RangeReadRequest

A single range read request with byte offset and length.

```cpp
struct RangeReadRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string reader_id;
    int64_t byte_offset{0};
    int64_t byte_length{0};
};
```

### RangeReadResult

Result of a range read operation with metadata.

```cpp
struct RangeReadResult {
    ContractMetadata metadata;
    std::string request_id;
    bool success{false};
    int64_t bytes_read{0};
    int64_t response_time_ms{0};
    int64_t total_content_length{0};
    std::string etag;
    std::string error_message;
};
```

## Fake Service

Provides deterministic range read results for testing:

- `build_range_reader()` - Configured network range reader
- `build_successful_range_read()` - Successful range read
- `build_range_not_satisfiable()` - Range not satisfiable (416)
- `build_partial_range_read()` - Partial content (206)
- `build_range_read_error()` - Connection error during read

## Tests

1. Range reader identity and capability
2. Successful range read with correct data
3. Range not satisfiable response
4. Partial content with metadata
5. Read error with error message

## Evidence Target

Verified range reader behind source adapter boundary.

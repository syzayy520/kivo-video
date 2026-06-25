# P2-029 Source Health / Remote Reliability

Updated: 2026-06-25

## Goal

Establish source health tracking and reliability metrics for remote sources. These provide the Inspector with visibility into source connection status, throughput, and reliability.

## Scope

Expand source_core/ with health status, reliability metrics, and health check types that feed into the Inspector's remote source monitoring.

## Contract Types

### SourceHealthStatus

Health status classification for a remote source.

```cpp
struct SourceHealthStatus {
    ContractMetadata metadata;
    std::string source_id;
    std::string status;  // "healthy", "degraded", "unreachable", "recovering"
    std::string status_reason;
    int64_t status_since_timestamp{0};
};
```

### SourceReliabilityMetrics

Reliability statistics for a remote source connection.

```cpp
struct SourceReliabilityMetrics {
    ContractMetadata metadata;
    std::string source_id;
    int64_t total_requests{0};
    int64_t successful_requests{0};
    int64_t failed_requests{0};
    double success_rate{0.0};  // 0.0-1.0
    int consecutive_failures{0};
    int64_t last_success_timestamp{0};
    int64_t average_latency_ms{0};
};
```

### SourceHealthCheck

A single health check operation against a remote source.

```cpp
struct SourceHealthCheck {
    ContractMetadata metadata;
    std::string check_id;
    std::string source_id;
    std::string check_type;  // "connectivity", "throughput", "auth", "range_read"
    bool passed{false};
    std::string message;
    int64_t check_timestamp{0};
    int64_t response_time_ms{0};
};
```

## Fake Service

Provides deterministic source health results for testing:

- `build_healthy_source()` - Source is fully healthy
- `build_degraded_source()` - Source has degraded performance
- `build_unreachable_source()` - Source is unreachable
- `build_recovering_source()` - Source was degraded but is recovering

## Tests

1. Healthy source with 100% success rate
2. Degraded source with high latency
3. Unreachable source with consecutive failures
4. Recovering source with improving metrics

## Evidence Target

source_health_core snapshots and Inspector integration.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::p3::observability {

/// MetricKind enumerates safe metrics kinds.
enum class MetricKind
{
    operation,
    source,
    provider,
    transport,
    delivery,
    latency_bucket,
    candidate,
    confidence,
    failure,
    retry,
    cancel,
    probe,
    manifest,
    playlist,
    handoff,
    revision
};

/// ObservabilityMetric defines a safe metric (no raw URL/path/username/token).
/// schema_version: p3.observability.metric.v1
struct ObservabilityMetric
{
    std::string schema_version{"p3.observability.metric.v1"};
    MetricKind kind{MetricKind::operation};
    std::string metric_id{};
    int64_t value{0};
    std::string unit{};
    std::string redacted_context{};
    bool blocks_success_path{false};

    bool is_safe() const noexcept
    {
        // No raw URL, path, username, token, cookie, signed URL, title, private host, display name
        return !blocks_success_path;
    }
};

/// ObservabilityMetricsPolicy defines metrics collection policy.
struct ObservabilityMetricsPolicy
{
    std::string schema_version{"p3.observability.policy.v1"};
    bool collect_latency_buckets{true};
    bool collect_failure_reasons{true};
    bool collect_retry_counts{true};
    bool collect_cancel_reasons{true};
    int32_t max_metrics_per_operation{100};
    bool redact_all_user_data{true};
    bool no_metrics_blocking_success{true};
};

} // namespace kivo::p3::observability

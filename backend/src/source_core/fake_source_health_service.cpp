#include "kivo/cinema_engine/source_core/fake_source_health_service.hpp"

namespace kivo::cinema_engine {

// --- Healthy Source ---

SourceHealthStatus FakeSourceHealthService::build_healthy_status() {
    SourceHealthStatus s;
    s.metadata.schema_version = "1.1";
    s.source_id = "healthy-001";
    s.status = "healthy";
    s.status_reason = "All health checks pass";
    s.status_since_timestamp = 1719360000;
    return s;
}

SourceReliabilityMetrics FakeSourceHealthService::build_healthy_metrics() {
    SourceReliabilityMetrics m;
    m.metadata.schema_version = "1.1";
    m.source_id = "healthy-001";
    m.total_requests = 1000;
    m.successful_requests = 1000;
    m.failed_requests = 0;
    m.success_rate = 1.0;
    m.consecutive_failures = 0;
    m.last_success_timestamp = 1719360000;
    m.average_latency_ms = 45;
    return m;
}

SourceHealthCheck FakeSourceHealthService::build_healthy_check() {
    SourceHealthCheck c;
    c.metadata.schema_version = "1.1";
    c.check_id = "check-healthy-001";
    c.source_id = "healthy-001";
    c.check_type = "connectivity";
    c.passed = true;
    c.message = "Connection established within 50ms";
    c.check_timestamp = 1719360000;
    c.response_time_ms = 48;
    return c;
}

// --- Degraded Source ---

SourceHealthStatus FakeSourceHealthService::build_degraded_status() {
    SourceHealthStatus s;
    s.metadata.schema_version = "1.1";
    s.source_id = "degraded-001";
    s.status = "degraded";
    s.status_reason = "High latency, throughput below threshold";
    s.status_since_timestamp = 1719360000;
    return s;
}

SourceReliabilityMetrics FakeSourceHealthService::build_degraded_metrics() {
    SourceReliabilityMetrics m;
    m.metadata.schema_version = "1.1";
    m.source_id = "degraded-001";
    m.total_requests = 500;
    m.successful_requests = 420;
    m.failed_requests = 80;
    m.success_rate = 0.84;
    m.consecutive_failures = 3;
    m.last_success_timestamp = 1719360000;
    m.average_latency_ms = 750;
    return m;
}

SourceHealthCheck FakeSourceHealthService::build_degraded_check() {
    SourceHealthCheck c;
    c.metadata.schema_version = "1.1";
    c.check_id = "check-degraded-001";
    c.source_id = "degraded-001";
    c.check_type = "throughput";
    c.passed = false;
    c.message = "Throughput below minimum threshold of 1MB/s";
    c.check_timestamp = 1719360000;
    c.response_time_ms = 1200;
    return c;
}

// --- Unreachable Source ---

SourceHealthStatus FakeSourceHealthService::build_unreachable_status() {
    SourceHealthStatus s;
    s.metadata.schema_version = "1.1";
    s.source_id = "unreachable-001";
    s.status = "unreachable";
    s.status_reason = "Connection failed after 3 retries";
    s.status_since_timestamp = 1719360000;
    return s;
}

SourceReliabilityMetrics FakeSourceHealthService::build_unreachable_metrics() {
    SourceReliabilityMetrics m;
    m.metadata.schema_version = "1.1";
    m.source_id = "unreachable-001";
    m.total_requests = 10;
    m.successful_requests = 0;
    m.failed_requests = 10;
    m.success_rate = 0.0;
    m.consecutive_failures = 10;
    m.last_success_timestamp = 0;
    m.average_latency_ms = 0;
    return m;
}

SourceHealthCheck FakeSourceHealthService::build_unreachable_check() {
    SourceHealthCheck c;
    c.metadata.schema_version = "1.1";
    c.check_id = "check-unreachable-001";
    c.source_id = "unreachable-001";
    c.check_type = "connectivity";
    c.passed = false;
    c.message = "Connection timeout after 30000ms";
    c.check_timestamp = 1719360000;
    c.response_time_ms = 30000;
    return c;
}

// --- Recovering Source ---

SourceHealthStatus FakeSourceHealthService::build_recovering_status() {
    SourceHealthStatus s;
    s.metadata.schema_version = "1.1";
    s.source_id = "recovering-001";
    s.status = "recovering";
    s.status_reason = "Latency dropping, success rate improving";
    s.status_since_timestamp = 1719360000;
    return s;
}

SourceReliabilityMetrics FakeSourceHealthService::build_recovering_metrics() {
    SourceReliabilityMetrics m;
    m.metadata.schema_version = "1.1";
    m.source_id = "recovering-001";
    m.total_requests = 100;
    m.successful_requests = 85;
    m.failed_requests = 15;
    m.success_rate = 0.85;
    m.consecutive_failures = 1;
    m.last_success_timestamp = 1719360000;
    m.average_latency_ms = 200;
    return m;
}

SourceHealthCheck FakeSourceHealthService::build_recovering_check() {
    SourceHealthCheck c;
    c.metadata.schema_version = "1.1";
    c.check_id = "check-recovering-001";
    c.source_id = "recovering-001";
    c.check_type = "range_read";
    c.passed = true;
    c.message = "Range read successful, response improving";
    c.check_timestamp = 1719360000;
    c.response_time_ms = 180;
    return c;
}

}  // namespace kivo::cinema_engine

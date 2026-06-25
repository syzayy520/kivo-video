#pragma once

#include "source_health_status.hpp"
#include "source_reliability_metrics.hpp"
#include "source_health_check.hpp"

namespace kivo::cinema_engine {

// Provides deterministic source health results for testing
class FakeSourceHealthService {
public:
    // Healthy source with 100% success rate
    static SourceHealthStatus build_healthy_status();
    static SourceReliabilityMetrics build_healthy_metrics();
    static SourceHealthCheck build_healthy_check();

    // Degraded source with high latency
    static SourceHealthStatus build_degraded_status();
    static SourceReliabilityMetrics build_degraded_metrics();
    static SourceHealthCheck build_degraded_check();

    // Unreachable source
    static SourceHealthStatus build_unreachable_status();
    static SourceReliabilityMetrics build_unreachable_metrics();
    static SourceHealthCheck build_unreachable_check();

    // Recovering source
    static SourceHealthStatus build_recovering_status();
    static SourceReliabilityMetrics build_recovering_metrics();
    static SourceHealthCheck build_recovering_check();
};

}  // namespace kivo::cinema_engine

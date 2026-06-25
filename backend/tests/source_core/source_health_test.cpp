#include "kivo/cinema_engine/source_core/fake_source_health_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_healthy_source() {
    auto status = FakeSourceHealthService::build_healthy_status();
    assert(status.source_id == "healthy-001");
    assert(status.status == "healthy");

    auto metrics = FakeSourceHealthService::build_healthy_metrics();
    assert(metrics.success_rate == 1.0);
    assert(metrics.failed_requests == 0);
    assert(metrics.consecutive_failures == 0);
    assert(metrics.average_latency_ms == 45);

    auto check = FakeSourceHealthService::build_healthy_check();
    assert(check.passed);
    assert(check.check_type == "connectivity");
    std::cout << "PASS: test_healthy_source" << std::endl;
}

void test_degraded_source() {
    auto status = FakeSourceHealthService::build_degraded_status();
    assert(status.status == "degraded");

    auto metrics = FakeSourceHealthService::build_degraded_metrics();
    assert(metrics.success_rate == 0.84);
    assert(metrics.failed_requests == 80);
    assert(metrics.consecutive_failures == 3);
    assert(metrics.average_latency_ms == 750);

    auto check = FakeSourceHealthService::build_degraded_check();
    assert(!check.passed);
    assert(check.check_type == "throughput");
    std::cout << "PASS: test_degraded_source" << std::endl;
}

void test_unreachable_source() {
    auto status = FakeSourceHealthService::build_unreachable_status();
    assert(status.status == "unreachable");

    auto metrics = FakeSourceHealthService::build_unreachable_metrics();
    assert(metrics.success_rate == 0.0);
    assert(metrics.successful_requests == 0);
    assert(metrics.consecutive_failures == 10);
    assert(metrics.last_success_timestamp == 0);

    auto check = FakeSourceHealthService::build_unreachable_check();
    assert(!check.passed);
    assert(check.response_time_ms == 30000);
    std::cout << "PASS: test_unreachable_source" << std::endl;
}

void test_recovering_source() {
    auto status = FakeSourceHealthService::build_recovering_status();
    assert(status.status == "recovering");
    assert(status.status_reason.find("improving") != std::string::npos);

    auto metrics = FakeSourceHealthService::build_recovering_metrics();
    assert(metrics.success_rate == 0.85);
    assert(metrics.consecutive_failures == 1);
    assert(metrics.average_latency_ms == 200);
    assert(metrics.last_success_timestamp > 0);

    auto check = FakeSourceHealthService::build_recovering_check();
    assert(check.passed);
    assert(check.check_type == "range_read");
    assert(check.response_time_ms == 180);
    std::cout << "PASS: test_recovering_source" << std::endl;
}

int main() {
    test_healthy_source();
    test_degraded_source();
    test_unreachable_source();
    test_recovering_source();

    std::cout << "All source health tests passed!" << std::endl;
    return 0;
}

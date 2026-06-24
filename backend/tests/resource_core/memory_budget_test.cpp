#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/resource_core/fake_memory_budget_service.hpp"

using namespace kivo::cinema_engine;

static void test_default_budget() {
    auto budget = build_default_memory_budget("test-budget");
    assert(budget.metadata.schema_version == "1.1");
    assert(budget.budget_id == "test-budget");
    assert(budget.total_budget_bytes == 256 * 1024 * 1024);
    assert(budget.video_queue_bytes == 0);
    assert(budget.audio_queue_bytes == 0);
    assert(budget.decode_buffer_bytes == 0);
    assert(budget.cache_bytes == 0);
    assert(budget.is_over_budget == false);
    assert(budget.utilization_pct == 0.0);
    std::cout << "  PASS: default_budget\n";
}

static void test_custom_budget() {
    auto budget = build_memory_budget("custom-budget", 512 * 1024 * 1024);
    assert(budget.budget_id == "custom-budget");
    assert(budget.total_budget_bytes == 512 * 1024 * 1024);
    assert(budget.is_over_budget == false);
    std::cout << "  PASS: custom_budget\n";
}

static void test_over_budget() {
    auto budget = build_over_budget_memory("over-budget");
    assert(budget.budget_id == "over-budget");
    assert(budget.total_budget_bytes == 100 * 1024 * 1024);
    assert(budget.video_queue_bytes == 80 * 1024 * 1024);
    assert(budget.audio_queue_bytes == 20 * 1024 * 1024);
    assert(budget.decode_buffer_bytes == 10 * 1024 * 1024);
    assert(budget.cache_bytes == 5 * 1024 * 1024);
    assert(budget.is_over_budget == true);
    assert(budget.utilization_pct == 115.0);
    std::cout << "  PASS: over_budget\n";
}

static void test_budget_with_usage() {
    auto budget = build_memory_budget_with_usage(
        "usage-budget",
        50 * 1024 * 1024,  // video
        10 * 1024 * 1024,  // audio
        20 * 1024 * 1024,  // decode
        5 * 1024 * 1024    // cache
    );
    assert(budget.budget_id == "usage-budget");
    assert(budget.total_budget_bytes == 256 * 1024 * 1024);
    assert(budget.video_queue_bytes == 50 * 1024 * 1024);
    assert(budget.audio_queue_bytes == 10 * 1024 * 1024);
    assert(budget.decode_buffer_bytes == 20 * 1024 * 1024);
    assert(budget.cache_bytes == 5 * 1024 * 1024);
    int64_t total_used = 50 + 10 + 20 + 5; // MB
    double expected_pct = (static_cast<double>(total_used * 1024 * 1024) / (256 * 1024 * 1024)) * 100.0;
    assert(budget.utilization_pct == expected_pct);
    assert(budget.is_over_budget == false);
    std::cout << "  PASS: budget_with_usage\n";
}

int main() {
    std::cout << "memory_budget_test:\n";
    test_default_budget();
    test_custom_budget();
    test_over_budget();
    test_budget_with_usage();
    std::cout << "ALL 4 TESTS PASSED\n";
    return 0;
}
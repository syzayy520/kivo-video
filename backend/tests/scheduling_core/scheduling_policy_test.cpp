#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/scheduling_core/fake_scheduling_service.hpp"

using namespace kivo::cinema_engine;

static void test_default_config() {
    auto config = build_default_scheduling_config("test-policy");
    assert(config.metadata.schema_version == "1.1");
    assert(config.policy_id == "test-policy");
    assert(config.task_category == "Playback");
    assert(config.priority_level == 2);
    assert(config.enable_mmcss == true);
    assert(config.fallback_on_failure == true);
    std::cout << "  PASS: default_config\n";
}

static void test_mmcss_enabled_config() {
    auto config = build_mmcss_enabled_config("mmcss-policy");
    assert(config.policy_id == "mmcss-policy");
    assert(config.priority_level == 5);
    assert(config.enable_mmcss == true);
    std::cout << "  PASS: mmcss_enabled_config\n";
}

static void test_mmcss_disabled_config() {
    auto config = build_mmcss_disabled_config("no-mmcss-policy");
    assert(config.policy_id == "no-mmcss-policy");
    assert(config.priority_level == 2);
    assert(config.enable_mmcss == false);
    std::cout << "  PASS: mmcss_disabled_config\n";
}

static void test_registered_status() {
    auto status = build_registered_status("test-policy");
    assert(status.policy_id == "test-policy");
    assert(status.mmcss_registered == true);
    assert(status.mmcss_available == true);
    assert(status.current_priority == 5);
    assert(status.last_error.empty());
    std::cout << "  PASS: registered_status\n";
}

static void test_failed_status() {
    auto status = build_failed_status("test-policy", "Access denied");
    assert(status.policy_id == "test-policy");
    assert(status.mmcss_registered == false);
    assert(status.mmcss_available == true);
    assert(status.current_priority == 0);
    assert(status.last_error == "Access denied");
    std::cout << "  PASS: failed_status\n";
}

static void test_unavailable_status() {
    auto status = build_unavailable_status("test-policy");
    assert(status.policy_id == "test-policy");
    assert(status.mmcss_registered == false);
    assert(status.mmcss_available == false);
    assert(status.current_priority == 0);
    assert(status.last_error == "MMCSS not available");
    std::cout << "  PASS: unavailable_status\n";
}

int main() {
    std::cout << "scheduling_policy_test:\n";
    test_default_config();
    test_mmcss_enabled_config();
    test_mmcss_disabled_config();
    test_registered_status();
    test_failed_status();
    test_unavailable_status();
    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}
#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/storage_core/fake_database_resilience_service.hpp"

using namespace kivo::cinema_engine;

void test_database_resilience_policy_construction() {
    auto policy = FakeDatabaseResilienceService::build_source_database_policy();
    assert(!policy.policy_id.empty());
    assert(!policy.database_type.empty());
    assert(policy.backup_interval_ms > 0);
    assert(policy.max_retry_attempts >= 0);
    std::cout << "PASS: test_database_resilience_policy_construction" << std::endl;
}

void test_database_health_check_construction() {
    auto check = FakeDatabaseResilienceService::build_healthy_check();
    assert(!check.check_id.empty());
    assert(!check.database_type.empty());
    assert(check.last_check_ms > 0);
    assert(check.response_time_ms >= 0);
    std::cout << "PASS: test_database_health_check_construction" << std::endl;
}

void test_database_backup_policy_construction() {
    auto policy = FakeDatabaseResilienceService::build_standard_backup_policy();
    assert(!policy.policy_id.empty());
    assert(!policy.database_type.empty());
    assert(!policy.backup_location.empty());
    assert(policy.retention_days > 0);
    std::cout << "PASS: test_database_backup_policy_construction" << std::endl;
}

void test_database_recovery_policy_construction() {
    auto policy = FakeDatabaseResilienceService::build_point_in_time_recovery();
    assert(!policy.policy_id.empty());
    assert(!policy.database_type.empty());
    assert(!policy.recovery_mode.empty());
    assert(!policy.data_loss_tolerance.empty());
    std::cout << "PASS: test_database_recovery_policy_construction" << std::endl;
}

void test_source_database_resilience_policy() {
    auto policy = FakeDatabaseResilienceService::build_source_database_policy();
    assert(policy.database_type == "source");
    assert(policy.backup_enabled == true);
    assert(policy.recovery_enabled == true);
    assert(policy.failover_enabled == true);
    std::cout << "PASS: test_source_database_resilience_policy" << std::endl;
}

void test_playback_database_resilience_policy() {
    auto policy = FakeDatabaseResilienceService::build_playback_database_policy();
    assert(policy.database_type == "playback");
    assert(policy.backup_enabled == false);
    assert(policy.recovery_enabled == true);
    assert(policy.failover_enabled == false);
    std::cout << "PASS: test_playback_database_resilience_policy" << std::endl;
}

void test_decision_database_resilience_policy() {
    auto policy = FakeDatabaseResilienceService::build_decision_database_policy();
    assert(policy.database_type == "decision");
    assert(policy.backup_enabled == true);
    assert(policy.recovery_enabled == true);
    assert(policy.failover_enabled == false);
    std::cout << "PASS: test_decision_database_resilience_policy" << std::endl;
}

void test_cache_database_resilience_policy() {
    auto policy = FakeDatabaseResilienceService::build_cache_database_policy();
    assert(policy.database_type == "cache");
    assert(policy.backup_enabled == false);
    assert(policy.recovery_enabled == false);
    assert(policy.failover_enabled == false);
    std::cout << "PASS: test_cache_database_resilience_policy" << std::endl;
}

void test_telemetry_database_resilience_policy() {
    auto policy = FakeDatabaseResilienceService::build_telemetry_database_policy();
    assert(policy.database_type == "telemetry");
    assert(policy.backup_enabled == true);
    assert(policy.recovery_enabled == true);
    assert(policy.failover_enabled == false);
    std::cout << "PASS: test_telemetry_database_resilience_policy" << std::endl;
}

void test_schema_database_resilience_policy() {
    auto policy = FakeDatabaseResilienceService::build_schema_database_policy();
    assert(policy.database_type == "schema");
    assert(policy.backup_enabled == true);
    assert(policy.recovery_enabled == true);
    assert(policy.failover_enabled == false);
    std::cout << "PASS: test_schema_database_resilience_policy" << std::endl;
}

int main() {
    test_database_resilience_policy_construction();
    test_database_health_check_construction();
    test_database_backup_policy_construction();
    test_database_recovery_policy_construction();
    test_source_database_resilience_policy();
    test_playback_database_resilience_policy();
    test_decision_database_resilience_policy();
    test_cache_database_resilience_policy();
    test_telemetry_database_resilience_policy();
    test_schema_database_resilience_policy();
    
    std::cout << "All database resilience tests passed." << std::endl;
    return 0;
}
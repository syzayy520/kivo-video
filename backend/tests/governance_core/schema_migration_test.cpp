#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/governance_core/fake_schema_migration_service.hpp"

using namespace kivo::cinema_engine;

void test_schema_migration_policy_construction() {
    auto policy = FakeSchemaMigrationService::build_forward_only_policy();
    assert(!policy.policy_id.empty());
    assert(!policy.source_schema_version.empty());
    assert(!policy.target_schema_version.empty());
    assert(policy.migration_timeout_ms > 0);
    std::cout << "PASS: test_schema_migration_policy_construction" << std::endl;
}

void test_schema_version_construction() {
    auto version = FakeSchemaMigrationService::build_current_version();
    assert(!version.version_id.empty());
    assert(!version.schema_name.empty());
    assert(version.created_at > 0);
    std::cout << "PASS: test_schema_version_construction" << std::endl;
}

void test_migration_step_construction() {
    auto step = FakeSchemaMigrationService::build_add_column_step();
    assert(!step.step_id.empty());
    assert(!step.source_version.empty());
    assert(!step.target_version.empty());
    assert(!step.migration_type.empty());
    assert(step.estimated_duration_ms > 0);
    std::cout << "PASS: test_migration_step_construction" << std::endl;
}

void test_compatibility_test_result_construction() {
    auto result = FakeSchemaMigrationService::build_passing_test();
    assert(!result.result_id.empty());
    assert(!result.source_version.empty());
    assert(!result.target_version.empty());
    assert(result.test_duration_ms > 0);
    std::cout << "PASS: test_compatibility_test_result_construction" << std::endl;
}

void test_breaking_changes_allowed_policy() {
    auto policy = FakeSchemaMigrationService::build_destructive_policy();
    assert(policy.allow_breaking_changes == true);
    assert(policy.require_compatibility_test == false);
    assert(policy.rollback_enabled == false);
    std::cout << "PASS: test_breaking_changes_allowed_policy" << std::endl;
}

void test_breaking_changes_forbidden_policy() {
    auto policy = FakeSchemaMigrationService::build_no_breaking_changes_policy();
    assert(policy.allow_breaking_changes == false);
    assert(policy.require_compatibility_test == true);
    assert(policy.rollback_enabled == true);
    std::cout << "PASS: test_breaking_changes_forbidden_policy" << std::endl;
}

void test_rollback_enabled_policy() {
    auto policy = FakeSchemaMigrationService::build_backward_compatible_policy();
    assert(policy.rollback_enabled == true);
    assert(policy.allow_breaking_changes == false);
    std::cout << "PASS: test_rollback_enabled_policy" << std::endl;
}

void test_rollback_disabled_policy() {
    auto policy = FakeSchemaMigrationService::build_forward_only_policy();
    assert(policy.rollback_enabled == false);
    assert(policy.allow_breaking_changes == false);
    std::cout << "PASS: test_rollback_disabled_policy" << std::endl;
}

void test_compatibility_test_pass_result() {
    auto result = FakeSchemaMigrationService::build_passing_test();
    assert(result.is_compatible == true);
    assert(result.errors_found == 0);
    assert(result.warnings_found == 0);
    std::cout << "PASS: test_compatibility_test_pass_result" << std::endl;
}

void test_compatibility_test_fail_result_with_errors() {
    auto result = FakeSchemaMigrationService::build_failing_test_with_errors();
    assert(result.is_compatible == false);
    assert(result.errors_found > 0);
    assert(result.warnings_found > 0);
    std::cout << "PASS: test_compatibility_test_fail_result_with_errors" << std::endl;
}

int main() {
    test_schema_migration_policy_construction();
    test_schema_version_construction();
    test_migration_step_construction();
    test_compatibility_test_result_construction();
    test_breaking_changes_allowed_policy();
    test_breaking_changes_forbidden_policy();
    test_rollback_enabled_policy();
    test_rollback_disabled_policy();
    test_compatibility_test_pass_result();
    test_compatibility_test_fail_result_with_errors();
    
    std::cout << "All schema migration tests passed." << std::endl;
    return 0;
}
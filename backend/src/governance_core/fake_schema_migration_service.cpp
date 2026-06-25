#include "kivo/cinema_engine/governance_core/fake_schema_migration_service.hpp"

namespace kivo::cinema_engine {

// Policy builders
SchemaMigrationPolicy FakeSchemaMigrationService::build_forward_only_policy() {
    SchemaMigrationPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "forward-only-001";
    policy.source_schema_version = "1.0.0";
    policy.target_schema_version = "1.1.0";
    policy.allow_breaking_changes = false;
    policy.require_compatibility_test = true;
    policy.rollback_enabled = false;
    policy.migration_timeout_ms = 30000;
    return policy;
}

SchemaMigrationPolicy FakeSchemaMigrationService::build_backward_compatible_policy() {
    SchemaMigrationPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "backward-compatible-001";
    policy.source_schema_version = "1.0.0";
    policy.target_schema_version = "2.0.0";
    policy.allow_breaking_changes = false;
    policy.require_compatibility_test = true;
    policy.rollback_enabled = true;
    policy.migration_timeout_ms = 60000;
    return policy;
}

SchemaMigrationPolicy FakeSchemaMigrationService::build_destructive_policy() {
    SchemaMigrationPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "destructive-001";
    policy.source_schema_version = "1.0.0";
    policy.target_schema_version = "3.0.0";
    policy.allow_breaking_changes = true;
    policy.require_compatibility_test = false;
    policy.rollback_enabled = false;
    policy.migration_timeout_ms = 10000;
    return policy;
}

SchemaMigrationPolicy FakeSchemaMigrationService::build_no_breaking_changes_policy() {
    SchemaMigrationPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "no-breaking-001";
    policy.source_schema_version = "1.0.0";
    policy.target_schema_version = "1.0.1";
    policy.allow_breaking_changes = false;
    policy.require_compatibility_test = true;
    policy.rollback_enabled = true;
    policy.migration_timeout_ms = 5000;
    return policy;
}

// Version builders
SchemaVersion FakeSchemaMigrationService::build_current_version() {
    SchemaVersion version;
    version.metadata.schema_version = "1.0.0";
    version.version_id = "1.0.0";
    version.schema_name = "playback_session";
    version.created_at = 1719312000000;  // 2024-06-25
    version.is_compatible = true;
    version.breaking_changes = 0;
    version.migration_count = 5;
    return version;
}

SchemaVersion FakeSchemaMigrationService::build_target_version() {
    SchemaVersion version;
    version.metadata.schema_version = "1.0.0";
    version.version_id = "1.1.0";
    version.schema_name = "playback_session";
    version.created_at = 1719398400000;  // 2024-06-26
    version.is_compatible = true;
    version.breaking_changes = 0;
    version.migration_count = 1;
    return version;
}

SchemaVersion FakeSchemaMigrationService::build_incompatible_version() {
    SchemaVersion version;
    version.metadata.schema_version = "1.0.0";
    version.version_id = "2.0.0";
    version.schema_name = "playback_session";
    version.created_at = 1719484800000;  // 2024-06-27
    version.is_compatible = false;
    version.breaking_changes = 3;
    version.migration_count = 2;
    return version;
}

SchemaVersion FakeSchemaMigrationService::build_version_with_breaking_changes() {
    SchemaVersion version;
    version.metadata.schema_version = "1.0.0";
    version.version_id = "1.5.0";
    version.schema_name = "playback_session";
    version.created_at = 1719571200000;  // 2024-06-28
    version.is_compatible = true;
    version.breaking_changes = 1;
    version.migration_count = 3;
    return version;
}

// Step builders
MigrationStep FakeSchemaMigrationService::build_add_column_step() {
    MigrationStep step;
    step.metadata.schema_version = "1.0.0";
    step.step_id = "add-column-001";
    step.source_version = "1.0.0";
    step.target_version = "1.1.0";
    step.migration_type = "add_column";
    step.is_reversible = true;
    step.estimated_duration_ms = 500;
    step.requires_downtime = false;
    return step;
}

MigrationStep FakeSchemaMigrationService::build_rename_table_step() {
    MigrationStep step;
    step.metadata.schema_version = "1.0.0";
    step.step_id = "rename-table-001";
    step.source_version = "1.0.0";
    step.target_version = "2.0.0";
    step.migration_type = "rename_table";
    step.is_reversible = true;
    step.estimated_duration_ms = 2000;
    step.requires_downtime = true;
    return step;
}

MigrationStep FakeSchemaMigrationService::build_irreversible_step() {
    MigrationStep step;
    step.metadata.schema_version = "1.0.0";
    step.step_id = "irreversible-001";
    step.source_version = "1.0.0";
    step.target_version = "3.0.0";
    step.migration_type = "drop_column";
    step.is_reversible = false;
    step.estimated_duration_ms = 1000;
    step.requires_downtime = true;
    return step;
}

MigrationStep FakeSchemaMigrationService::build_downtime_required_step() {
    MigrationStep step;
    step.metadata.schema_version = "1.0.0";
    step.step_id = "downtime-001";
    step.source_version = "1.0.0";
    step.target_version = "1.2.0";
    step.migration_type = "recreate_index";
    step.is_reversible = true;
    step.estimated_duration_ms = 5000;
    step.requires_downtime = true;
    return step;
}

// Test result builders
CompatibilityTestResult FakeSchemaMigrationService::build_passing_test() {
    CompatibilityTestResult result;
    result.metadata.schema_version = "1.0.0";
    result.result_id = "pass-001";
    result.source_version = "1.0.0";
    result.target_version = "1.1.0";
    result.is_compatible = true;
    result.test_duration_ms = 1000;
    result.errors_found = 0;
    result.warnings_found = 0;
    return result;
}

CompatibilityTestResult FakeSchemaMigrationService::build_failing_test_with_errors() {
    CompatibilityTestResult result;
    result.metadata.schema_version = "1.0.0";
    result.result_id = "fail-001";
    result.source_version = "1.0.0";
    result.target_version = "2.0.0";
    result.is_compatible = false;
    result.test_duration_ms = 2000;
    result.errors_found = 5;
    result.warnings_found = 2;
    return result;
}

CompatibilityTestResult FakeSchemaMigrationService::build_warning_test() {
    CompatibilityTestResult result;
    result.metadata.schema_version = "1.0.0";
    result.result_id = "warning-001";
    result.source_version = "1.0.0";
    result.target_version = "1.2.0";
    result.is_compatible = true;
    result.test_duration_ms = 1500;
    result.errors_found = 0;
    result.warnings_found = 3;
    return result;
}

CompatibilityTestResult FakeSchemaMigrationService::build_slow_test() {
    CompatibilityTestResult result;
    result.metadata.schema_version = "1.0.0";
    result.result_id = "slow-001";
    result.source_version = "1.0.0";
    result.target_version = "1.3.0";
    result.is_compatible = true;
    result.test_duration_ms = 10000;
    result.errors_found = 0;
    result.warnings_found = 1;
    return result;
}

}  // namespace kivo::cinema_engine
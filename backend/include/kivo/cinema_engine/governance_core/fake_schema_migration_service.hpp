#pragma once

#include "kivo/cinema_engine/governance_core/schema_migration_policy.hpp"
#include "kivo/cinema_engine/governance_core/schema_version.hpp"
#include "kivo/cinema_engine/governance_core/migration_step.hpp"
#include "kivo/cinema_engine/governance_core/compatibility_test_result.hpp"

namespace kivo::cinema_engine {

// Fake schema migration service for testing.
// Provides static builder functions for schema migration policy, version, step, and test result.
class FakeSchemaMigrationService {
 public:
    // Policy builders
    static SchemaMigrationPolicy build_forward_only_policy();
    static SchemaMigrationPolicy build_backward_compatible_policy();
    static SchemaMigrationPolicy build_destructive_policy();
    static SchemaMigrationPolicy build_no_breaking_changes_policy();
    
    // Version builders
    static SchemaVersion build_current_version();
    static SchemaVersion build_target_version();
    static SchemaVersion build_incompatible_version();
    static SchemaVersion build_version_with_breaking_changes();
    
    // Step builders
    static MigrationStep build_add_column_step();
    static MigrationStep build_rename_table_step();
    static MigrationStep build_irreversible_step();
    static MigrationStep build_downtime_required_step();
    
    // Test result builders
    static CompatibilityTestResult build_passing_test();
    static CompatibilityTestResult build_failing_test_with_errors();
    static CompatibilityTestResult build_warning_test();
    static CompatibilityTestResult build_slow_test();
};

}  // namespace kivo::cinema_engine
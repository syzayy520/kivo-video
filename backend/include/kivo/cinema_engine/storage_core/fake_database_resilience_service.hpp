#pragma once

#include "kivo/cinema_engine/storage_core/database_resilience_policy.hpp"
#include "kivo/cinema_engine/storage_core/database_health_check.hpp"
#include "kivo/cinema_engine/storage_core/database_backup_policy.hpp"
#include "kivo/cinema_engine/storage_core/database_recovery_policy.hpp"

namespace kivo::cinema_engine {

// Fake database resilience service for testing.
// Provides static builder functions for resilience policy, health check, backup, and recovery.
class FakeDatabaseResilienceService {
 public:
    // Policy builders
    static DatabaseResiliencePolicy build_source_database_policy();
    static DatabaseResiliencePolicy build_playback_database_policy();
    static DatabaseResiliencePolicy build_decision_database_policy();
    static DatabaseResiliencePolicy build_cache_database_policy();
    static DatabaseResiliencePolicy build_telemetry_database_policy();
    static DatabaseResiliencePolicy build_preference_database_policy();
    static DatabaseResiliencePolicy build_schema_database_policy();
    
    // Health check builders
    static DatabaseHealthCheck build_healthy_check();
    static DatabaseHealthCheck build_unhealthy_check();
    static DatabaseHealthCheck build_degraded_check();
    
    // Backup policy builders
    static DatabaseBackupPolicy build_standard_backup_policy();
    static DatabaseBackupPolicy build_encrypted_backup_policy();
    static DatabaseBackupPolicy build_long_retention_policy();
    
    // Recovery policy builders
    static DatabaseRecoveryPolicy build_point_in_time_recovery();
    static DatabaseRecoveryPolicy build_last_good_recovery();
    static DatabaseRecoveryPolicy build_manual_recovery();
};

}  // namespace kivo::cinema_engine
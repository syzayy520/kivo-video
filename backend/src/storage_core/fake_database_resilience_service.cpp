#include "kivo/cinema_engine/storage_core/fake_database_resilience_service.hpp"

namespace kivo::cinema_engine {

// Policy builders
DatabaseResiliencePolicy FakeDatabaseResilienceService::build_source_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "source-db-resilience-001";
    policy.database_type = "source";
    policy.backup_enabled = true;
    policy.backup_interval_ms = 1800000;  // 30 minutes
    policy.recovery_enabled = true;
    policy.max_retry_attempts = 5;
    policy.retry_delay_ms = 2000;
    policy.failover_enabled = true;
    policy.health_check_enabled = true;
    return policy;
}

DatabaseResiliencePolicy FakeDatabaseResilienceService::build_playback_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "playback-db-resilience-001";
    policy.database_type = "playback";
    policy.backup_enabled = false;
    policy.backup_interval_ms = 0;
    policy.recovery_enabled = true;
    policy.max_retry_attempts = 3;
    policy.retry_delay_ms = 1000;
    policy.failover_enabled = false;
    policy.health_check_enabled = true;
    return policy;
}

DatabaseResiliencePolicy FakeDatabaseResilienceService::build_decision_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "decision-db-resilience-001";
    policy.database_type = "decision";
    policy.backup_enabled = true;
    policy.backup_interval_ms = 3600000;  // 1 hour
    policy.recovery_enabled = true;
    policy.max_retry_attempts = 3;
    policy.retry_delay_ms = 1000;
    policy.failover_enabled = false;
    policy.health_check_enabled = true;
    return policy;
}

DatabaseResiliencePolicy FakeDatabaseResilienceService::build_cache_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "cache-db-resilience-001";
    policy.database_type = "cache";
    policy.backup_enabled = false;
    policy.backup_interval_ms = 0;
    policy.recovery_enabled = false;
    policy.max_retry_attempts = 0;
    policy.retry_delay_ms = 0;
    policy.failover_enabled = false;
    policy.health_check_enabled = false;
    return policy;
}

DatabaseResiliencePolicy FakeDatabaseResilienceService::build_telemetry_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "telemetry-db-resilience-001";
    policy.database_type = "telemetry";
    policy.backup_enabled = true;
    policy.backup_interval_ms = 86400000;  // 24 hours
    policy.recovery_enabled = true;
    policy.max_retry_attempts = 2;
    policy.retry_delay_ms = 5000;
    policy.failover_enabled = false;
    policy.health_check_enabled = true;
    return policy;
}

DatabaseResiliencePolicy FakeDatabaseResilienceService::build_preference_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "preference-db-resilience-001";
    policy.database_type = "preference";
    policy.backup_enabled = true;
    policy.backup_interval_ms = 7200000;  // 2 hours
    policy.recovery_enabled = true;
    policy.max_retry_attempts = 3;
    policy.retry_delay_ms = 1000;
    policy.failover_enabled = false;
    policy.health_check_enabled = true;
    return policy;
}

DatabaseResiliencePolicy FakeDatabaseResilienceService::build_schema_database_policy() {
    DatabaseResiliencePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "schema-db-resilience-001";
    policy.database_type = "schema";
    policy.backup_enabled = true;
    policy.backup_interval_ms = 3600000;  // 1 hour
    policy.recovery_enabled = true;
    policy.max_retry_attempts = 1;
    policy.retry_delay_ms = 10000;
    policy.failover_enabled = false;
    policy.health_check_enabled = true;
    return policy;
}

// Health check builders
DatabaseHealthCheck FakeDatabaseResilienceService::build_healthy_check() {
    DatabaseHealthCheck check;
    check.metadata.schema_version = "1.0.0";
    check.check_id = "healthy-001";
    check.database_type = "source";
    check.is_healthy = true;
    check.last_check_ms = 1719312000000;  // 2024-06-25
    check.response_time_ms = 10;
    check.error_count = 0;
    return check;
}

DatabaseHealthCheck FakeDatabaseResilienceService::build_unhealthy_check() {
    DatabaseHealthCheck check;
    check.metadata.schema_version = "1.0.0";
    check.check_id = "unhealthy-001";
    check.database_type = "playback";
    check.is_healthy = false;
    check.last_check_ms = 1719312000000;
    check.response_time_ms = 5000;
    check.error_count = 5;
    return check;
}

DatabaseHealthCheck FakeDatabaseResilienceService::build_degraded_check() {
    DatabaseHealthCheck check;
    check.metadata.schema_version = "1.0.0";
    check.check_id = "degraded-001";
    check.database_type = "decision";
    check.is_healthy = true;
    check.last_check_ms = 1719312000000;
    check.response_time_ms = 1000;
    check.error_count = 2;
    return check;
}

// Backup policy builders
DatabaseBackupPolicy FakeDatabaseResilienceService::build_standard_backup_policy() {
    DatabaseBackupPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "standard-backup-001";
    policy.database_type = "source";
    policy.backup_location = "/backups/source_db";
    policy.retention_days = 30;
    policy.compression_enabled = true;
    policy.encryption_enabled = false;
    return policy;
}

DatabaseBackupPolicy FakeDatabaseResilienceService::build_encrypted_backup_policy() {
    DatabaseBackupPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "encrypted-backup-001";
    policy.database_type = "decision";
    policy.backup_location = "/backups/decision_db_encrypted";
    policy.retention_days = 90;
    policy.compression_enabled = true;
    policy.encryption_enabled = true;
    return policy;
}

DatabaseBackupPolicy FakeDatabaseResilienceService::build_long_retention_policy() {
    DatabaseBackupPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "long-retention-001";
    policy.database_type = "schema";
    policy.backup_location = "/backups/schema_db_long";
    policy.retention_days = 365;
    policy.compression_enabled = true;
    policy.encryption_enabled = true;
    return policy;
}

// Recovery policy builders
DatabaseRecoveryPolicy FakeDatabaseResilienceService::build_point_in_time_recovery() {
    DatabaseRecoveryPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "point-in-time-001";
    policy.database_type = "source";
    policy.recovery_mode = "point-in-time";
    policy.max_recovery_time_ms = 300000;  // 5 minutes
    policy.data_loss_tolerance = "none";
    policy.verification_enabled = true;
    return policy;
}

DatabaseRecoveryPolicy FakeDatabaseResilienceService::build_last_good_recovery() {
    DatabaseRecoveryPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "last-good-001";
    policy.database_type = "playback";
    policy.recovery_mode = "last-good";
    policy.max_recovery_time_ms = 60000;  // 1 minute
    policy.data_loss_tolerance = "minimal";
    policy.verification_enabled = false;
    return policy;
}

DatabaseRecoveryPolicy FakeDatabaseResilienceService::build_manual_recovery() {
    DatabaseRecoveryPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "manual-001";
    policy.database_type = "schema";
    policy.recovery_mode = "manual";
    policy.max_recovery_time_ms = 0;  // No limit
    policy.data_loss_tolerance = "moderate";
    policy.verification_enabled = true;
    return policy;
}

}  // namespace kivo::cinema_engine
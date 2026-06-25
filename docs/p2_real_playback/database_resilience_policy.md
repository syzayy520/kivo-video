# P2-045 Database Resilience Policy Skeleton

## Purpose

Defines database resilience policy skeleton for source/playback/decision/cache/telemetry/preference/schema databases, covering backup, recovery, retry, and failover rules.

## Scope

- Database resilience rules for each database type
- Backup and recovery policies
- Retry and timeout configurations
- Failover and redundancy strategies
- Health monitoring and alerting

## Family Tree (storage_core/)

```
storage_core/
├── database_resilience_policy.hpp      — Resilience policy configuration
├── database_health_check.hpp           — Health check configuration
├── database_backup_policy.hpp          — Backup policy configuration
├── database_recovery_policy.hpp        — Recovery policy configuration
├── fake_database_resilience_service.hpp — Fake service for testing
```

## Contract Types

### DatabaseResiliencePolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| database_type | string | Database type (source, playback, decision, cache, telemetry, preference, schema) |
| backup_enabled | bool | Whether backup is enabled |
| backup_interval_ms | int64_t | Backup interval in milliseconds |
| recovery_enabled | bool | Whether recovery is enabled |
| max_retry_attempts | int | Maximum retry attempts |
| retry_delay_ms | int64_t | Delay between retries |
| failover_enabled | bool | Whether failover is enabled |
| health_check_enabled | bool | Whether health checks are enabled |

### DatabaseHealthCheck

| Field | Type | Description |
|-------|------|-------------|
| check_id | string | Unique check identifier |
| database_type | string | Database type |
| is_healthy | bool | Whether database is healthy |
| last_check_ms | int64_t | Last check timestamp |
| response_time_ms | int64_t | Response time |
| error_count | int | Number of consecutive errors |

### DatabaseBackupPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| database_type | string | Database type |
| backup_location | string | Backup storage location |
| retention_days | int | Backup retention period |
| compression_enabled | bool | Whether compression is enabled |
| encryption_enabled | bool | Whether encryption is enabled |

### DatabaseRecoveryPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| database_type | string | Database type |
| recovery_mode | string | Recovery mode (point-in-time, last-good, manual) |
| max_recovery_time_ms | int64_t | Maximum recovery time |
| data_loss_tolerance | string | Data loss tolerance (none, minimal, moderate) |
| verification_enabled | bool | Whether recovery verification is enabled |

## Tests

10 tests covering:
1. DatabaseResiliencePolicy construction with default values
2. DatabaseHealthCheck construction with healthy status
3. DatabaseBackupPolicy construction with encryption
4. DatabaseRecoveryPolicy construction with point-in-time recovery
5. Source database resilience policy
6. Playback database resilience policy
7. Decision database resilience policy
8. Cache database resilience policy
9. Telemetry database resilience policy
10. Schema database resilience policy
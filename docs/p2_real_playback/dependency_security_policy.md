# P2-047 Dependency Security / SBOM Policy Draft

## Purpose

Defines dependency security policy and SBOM (Software Bill of Materials) draft, including third-party dependency lock, CVE policy, and vulnerability management.

## Scope

- Third-party dependency lock rules
- SBOM generation and format
- CVE policy and vulnerability management
- License compliance
- Dependency update policies

## Family Tree (compliance_core/)

```
compliance_core/
├── dependency_security_policy.hpp       — Dependency security configuration
├── sbom_record.hpp                     — Software Bill of Materials record
├── cve_policy.hpp                      — CVE policy configuration
├── fake_dependency_security_service.hpp — Fake service for testing
```

## Contract Types

### DependencySecurityPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| dependency_lock_enabled | bool | Whether dependency lock is enabled |
| sbom_generation_enabled | bool | Whether SBOM generation is enabled |
| sbom_format | string | SBOM format (spdx, cyclonedx, swid) |
| cve_scan_enabled | bool | Whether CVE scanning is enabled |
| max_severity_allowed | string | Maximum severity allowed (critical, high, medium, low) |
| auto_update_enabled | bool | Whether automatic updates are enabled |
| require_license_compliance | bool | Whether license compliance is required |

### SbomRecord

| Field | Type | Description |
|-------|------|-------------|
| record_id | string | Unique record identifier |
| component_name | string | Component name |
| component_version | string | Component version |
| supplier | string | Component supplier |
| license | string | License identifier |
| hash | string | Component hash (SHA256) |
| is_vulnerable | bool | Whether component has known vulnerabilities |

### CvePolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| scan_frequency_hours | int | CVE scan frequency in hours |
| auto_patch_enabled | bool | Whether automatic patching is enabled |
| notification_enabled | bool | Whether notifications are enabled |
| max_response_time_hours | int | Maximum response time for critical CVEs |
| require_remediation_plan | bool | Whether remediation plan is required |

## Tests

10 tests covering:
1. DependencySecurityPolicy construction with default values
2. SbomRecord construction with SPDX format
3. CvePolicy construction with standard settings
4. Dependency lock enabled policy
5. Dependency lock disabled policy
6. SBOM generation with CycloneDX format
7. CVE scan with critical severity allowed
8. CVE scan with medium severity allowed
9. License compliance required policy
10. License compliance not required policy
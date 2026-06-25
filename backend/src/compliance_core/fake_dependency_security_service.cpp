#include "kivo/cinema_engine/compliance_core/fake_dependency_security_service.hpp"

namespace kivo::cinema_engine {

// Dependency security policy builders
DependencySecurityPolicy FakeDependencySecurityService::build_strict_security_policy() {
    DependencySecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "strict-security-001";
    policy.dependency_lock_enabled = true;
    policy.sbom_generation_enabled = true;
    policy.sbom_format = "spdx";
    policy.cve_scan_enabled = true;
    policy.max_severity_allowed = "low";
    policy.auto_update_enabled = false;
    policy.require_license_compliance = true;
    return policy;
}

DependencySecurityPolicy FakeDependencySecurityService::build_relaxed_security_policy() {
    DependencySecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "relaxed-security-001";
    policy.dependency_lock_enabled = true;
    policy.sbom_generation_enabled = false;
    policy.sbom_format = "cyclonedx";
    policy.cve_scan_enabled = false;
    policy.max_severity_allowed = "critical";
    policy.auto_update_enabled = true;
    policy.require_license_compliance = false;
    return policy;
}

DependencySecurityPolicy FakeDependencySecurityService::build_sbom_only_policy() {
    DependencySecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "sbom-only-001";
    policy.dependency_lock_enabled = true;
    policy.sbom_generation_enabled = true;
    policy.sbom_format = "spdx";
    policy.cve_scan_enabled = false;
    policy.max_severity_allowed = "critical";
    policy.auto_update_enabled = false;
    policy.require_license_compliance = true;
    return policy;
}

DependencySecurityPolicy FakeDependencySecurityService::build_cve_only_policy() {
    DependencySecurityPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "cve-only-001";
    policy.dependency_lock_enabled = true;
    policy.sbom_generation_enabled = false;
    policy.sbom_format = "";
    policy.cve_scan_enabled = true;
    policy.max_severity_allowed = "high";
    policy.auto_update_enabled = true;
    policy.require_license_compliance = false;
    return policy;
}

// SBOM record builders
SbomRecord FakeDependencySecurityService::build_safe_component() {
    SbomRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "sbom-safe-001";
    record.component_name = "openssl";
    record.component_version = "3.1.0";
    record.supplier = "OpenSSL Software Foundation";
    record.license = "Apache-2.0";
    record.hash = "sha256:abcdef1234567890";
    record.is_vulnerable = false;
    return record;
}

SbomRecord FakeDependencySecurityService::build_vulnerable_component() {
    SbomRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "sbom-vulnerable-001";
    record.component_name = "libpng";
    record.component_version = "1.6.37";
    record.supplier = "libpng.org";
    record.license = "libpng";
    record.hash = "sha256:fedcba0987654321";
    record.is_vulnerable = true;
    return record;
}

SbomRecord FakeDependencySecurityService::build_unknown_license_component() {
    SbomRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "sbom-unknown-license-001";
    record.component_name = "custom-library";
    record.component_version = "2.0.0";
    record.supplier = "internal-team";
    record.license = "UNKNOWN";
    record.hash = "sha256:1122334455667788";
    record.is_vulnerable = false;
    return record;
}

SbomRecord FakeDependencySecurityService::build_compliant_component() {
    SbomRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "sbom-compliant-001";
    record.component_name = "zlib";
    record.component_version = "1.2.13";
    record.supplier = "zlib.net";
    record.license = "Zlib";
    record.hash = "sha256:aabbccddee112233";
    record.is_vulnerable = false;
    return record;
}

// CVE policy builders
CvePolicy FakeDependencySecurityService::build_standard_cve_policy() {
    CvePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "standard-cve-001";
    policy.scan_frequency_hours = 24;
    policy.auto_patch_enabled = false;
    policy.notification_enabled = true;
    policy.max_response_time_hours = 72;
    policy.require_remediation_plan = true;
    return policy;
}

CvePolicy FakeDependencySecurityService::build_aggressive_cve_policy() {
    CvePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "aggressive-cve-001";
    policy.scan_frequency_hours = 6;
    policy.auto_patch_enabled = true;
    policy.notification_enabled = true;
    policy.max_response_time_hours = 24;
    policy.require_remediation_plan = true;
    return policy;
}

CvePolicy FakeDependencySecurityService::build_passive_cve_policy() {
    CvePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "passive-cve-001";
    policy.scan_frequency_hours = 168;  // weekly
    policy.auto_patch_enabled = false;
    policy.notification_enabled = false;
    policy.max_response_time_hours = 336;  // 2 weeks
    policy.require_remediation_plan = false;
    return policy;
}

CvePolicy FakeDependencySecurityService::build_enterprise_cve_policy() {
    CvePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "enterprise-cve-001";
    policy.scan_frequency_hours = 12;
    policy.auto_patch_enabled = false;
    policy.notification_enabled = true;
    policy.max_response_time_hours = 48;
    policy.require_remediation_plan = true;
    return policy;
}

}  // namespace kivo::cinema_engine
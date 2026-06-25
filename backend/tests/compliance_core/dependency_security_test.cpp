#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/compliance_core/fake_dependency_security_service.hpp"

namespace {

using namespace kivo::cinema_engine;

void test_strict_security_policy() {
    auto policy = FakeDependencySecurityService::build_strict_security_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "strict-security-001");
    assert(policy.dependency_lock_enabled == true);
    assert(policy.sbom_generation_enabled == true);
    assert(policy.sbom_format == "spdx");
    assert(policy.cve_scan_enabled == true);
    assert(policy.max_severity_allowed == "low");
    assert(policy.auto_update_enabled == false);
    assert(policy.require_license_compliance == true);
}

void test_relaxed_security_policy() {
    auto policy = FakeDependencySecurityService::build_relaxed_security_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "relaxed-security-001");
    assert(policy.dependency_lock_enabled == true);
    assert(policy.sbom_generation_enabled == false);
    assert(policy.sbom_format == "cyclonedx");
    assert(policy.cve_scan_enabled == false);
    assert(policy.max_severity_allowed == "critical");
    assert(policy.auto_update_enabled == true);
    assert(policy.require_license_compliance == false);
}

void test_sbom_only_policy() {
    auto policy = FakeDependencySecurityService::build_sbom_only_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "sbom-only-001");
    assert(policy.dependency_lock_enabled == true);
    assert(policy.sbom_generation_enabled == true);
    assert(policy.sbom_format == "spdx");
    assert(policy.cve_scan_enabled == false);
    assert(policy.max_severity_allowed == "critical");
    assert(policy.auto_update_enabled == false);
    assert(policy.require_license_compliance == true);
}

void test_cve_only_policy() {
    auto policy = FakeDependencySecurityService::build_cve_only_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "cve-only-001");
    assert(policy.dependency_lock_enabled == true);
    assert(policy.sbom_generation_enabled == false);
    assert(policy.sbom_format.empty());
    assert(policy.cve_scan_enabled == true);
    assert(policy.max_severity_allowed == "high");
    assert(policy.auto_update_enabled == true);
    assert(policy.require_license_compliance == false);
}

void test_safe_component() {
    auto record = FakeDependencySecurityService::build_safe_component();
    assert(record.metadata.schema_version == "1.0.0");
    assert(record.record_id == "sbom-safe-001");
    assert(record.component_name == "openssl");
    assert(record.component_version == "3.1.0");
    assert(record.supplier == "OpenSSL Software Foundation");
    assert(record.license == "Apache-2.0");
    assert(record.hash == "sha256:abcdef1234567890");
    assert(record.is_vulnerable == false);
}

void test_vulnerable_component() {
    auto record = FakeDependencySecurityService::build_vulnerable_component();
    assert(record.metadata.schema_version == "1.0.0");
    assert(record.record_id == "sbom-vulnerable-001");
    assert(record.component_name == "libpng");
    assert(record.component_version == "1.6.37");
    assert(record.supplier == "libpng.org");
    assert(record.license == "libpng");
    assert(record.hash == "sha256:fedcba0987654321");
    assert(record.is_vulnerable == true);
}

void test_unknown_license_component() {
    auto record = FakeDependencySecurityService::build_unknown_license_component();
    assert(record.metadata.schema_version == "1.0.0");
    assert(record.record_id == "sbom-unknown-license-001");
    assert(record.component_name == "custom-library");
    assert(record.component_version == "2.0.0");
    assert(record.supplier == "internal-team");
    assert(record.license == "UNKNOWN");
    assert(record.hash == "sha256:1122334455667788");
    assert(record.is_vulnerable == false);
}

void test_compliant_component() {
    auto record = FakeDependencySecurityService::build_compliant_component();
    assert(record.metadata.schema_version == "1.0.0");
    assert(record.record_id == "sbom-compliant-001");
    assert(record.component_name == "zlib");
    assert(record.component_version == "1.2.13");
    assert(record.supplier == "zlib.net");
    assert(record.license == "Zlib");
    assert(record.hash == "sha256:aabbccddee112233");
    assert(record.is_vulnerable == false);
}

void test_standard_cve_policy() {
    auto policy = FakeDependencySecurityService::build_standard_cve_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "standard-cve-001");
    assert(policy.scan_frequency_hours == 24);
    assert(policy.auto_patch_enabled == false);
    assert(policy.notification_enabled == true);
    assert(policy.max_response_time_hours == 72);
    assert(policy.require_remediation_plan == true);
}

void test_aggressive_cve_policy() {
    auto policy = FakeDependencySecurityService::build_aggressive_cve_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "aggressive-cve-001");
    assert(policy.scan_frequency_hours == 6);
    assert(policy.auto_patch_enabled == true);
    assert(policy.notification_enabled == true);
    assert(policy.max_response_time_hours == 24);
    assert(policy.require_remediation_plan == true);
}

void test_passive_cve_policy() {
    auto policy = FakeDependencySecurityService::build_passive_cve_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "passive-cve-001");
    assert(policy.scan_frequency_hours == 168);
    assert(policy.auto_patch_enabled == false);
    assert(policy.notification_enabled == false);
    assert(policy.max_response_time_hours == 336);
    assert(policy.require_remediation_plan == false);
}

void test_enterprise_cve_policy() {
    auto policy = FakeDependencySecurityService::build_enterprise_cve_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "enterprise-cve-001");
    assert(policy.scan_frequency_hours == 12);
    assert(policy.auto_patch_enabled == false);
    assert(policy.notification_enabled == true);
    assert(policy.max_response_time_hours == 48);
    assert(policy.require_remediation_plan == true);
}

}  // namespace

int main() {
    std::cout << "Running dependency security tests..." << std::endl;
    
    test_strict_security_policy();
    test_relaxed_security_policy();
    test_sbom_only_policy();
    test_cve_only_policy();
    test_safe_component();
    test_vulnerable_component();
    test_unknown_license_component();
    test_compliant_component();
    test_standard_cve_policy();
    test_aggressive_cve_policy();
    test_passive_cve_policy();
    test_enterprise_cve_policy();
    
    std::cout << "All 12 dependency security tests passed." << std::endl;
    return 0;
}
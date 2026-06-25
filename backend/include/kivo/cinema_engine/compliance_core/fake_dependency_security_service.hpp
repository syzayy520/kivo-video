#pragma once

#include "kivo/cinema_engine/compliance_core/dependency_security_policy.hpp"
#include "kivo/cinema_engine/compliance_core/sbom_record.hpp"
#include "kivo/cinema_engine/compliance_core/cve_policy.hpp"

namespace kivo::cinema_engine {

// Fake dependency security service for testing.
// Provides static builder functions for dependency security policy, SBOM, and CVE policy.
class FakeDependencySecurityService {
 public:
    // Policy builders
    static DependencySecurityPolicy build_strict_security_policy();
    static DependencySecurityPolicy build_relaxed_security_policy();
    static DependencySecurityPolicy build_sbom_only_policy();
    static DependencySecurityPolicy build_cve_only_policy();
    
    // SBOM record builders
    static SbomRecord build_safe_component();
    static SbomRecord build_vulnerable_component();
    static SbomRecord build_unknown_license_component();
    static SbomRecord build_compliant_component();
    
    // CVE policy builders
    static CvePolicy build_standard_cve_policy();
    static CvePolicy build_aggressive_cve_policy();
    static CvePolicy build_passive_cve_policy();
    static CvePolicy build_enterprise_cve_policy();
};

}  // namespace kivo::cinema_engine
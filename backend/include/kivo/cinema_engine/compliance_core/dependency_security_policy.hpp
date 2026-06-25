#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Dependency security policy configuration.
// Captures rules for third-party dependency lock, SBOM generation, and CVE management.
struct DependencySecurityPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    bool dependency_lock_enabled{true};  // Whether dependency lock is enabled
    bool sbom_generation_enabled{true};  // Whether SBOM generation is enabled
    std::string sbom_format;  // SBOM format (spdx, cyclonedx, swid)
    bool cve_scan_enabled{true};  // Whether CVE scanning is enabled
    std::string max_severity_allowed;  // Maximum severity allowed (critical, high, medium, low)
    bool auto_update_enabled{false};  // Whether automatic updates are enabled
    bool require_license_compliance{true};  // Whether license compliance is required
};

}  // namespace kivo::cinema_engine
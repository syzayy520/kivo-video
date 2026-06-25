#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// CVE policy configuration.
// Captures vulnerability scanning, patching, and response rules.
struct CvePolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    int scan_frequency_hours{24};  // CVE scan frequency in hours
    bool auto_patch_enabled{false};  // Whether automatic patching is enabled
    bool notification_enabled{true};  // Whether notifications are enabled
    int max_response_time_hours{72};  // Maximum response time for critical CVEs
    bool require_remediation_plan{true};  // Whether remediation plan is required
};

}  // namespace kivo::cinema_engine
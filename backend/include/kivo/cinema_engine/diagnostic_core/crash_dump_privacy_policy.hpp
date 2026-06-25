#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Crash dump privacy policy configuration.
// Captures privacy rules for crash dump collection, redaction, and retention.
struct CrashDumpPrivacyPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    bool collect_crash_dumps{true};  // Whether crash dumps are collected
    bool redact_pii{true};  // Whether PII is redacted
    bool redact_credentials{true};  // Whether credentials are redacted
    bool fail_closed_on_redaction_failure{true};  // Whether to fail closed if redaction fails
    int retention_days{30};  // Crash dump retention period
    bool require_user_consent{true};  // Whether user consent is required
};

}  // namespace kivo::cinema_engine
#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Support bundle redaction policy configuration.
// Captures redaction rules for support bundle generation.
struct SupportBundleRedactionPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    bool redact_user_paths{true};  // Whether user file paths are redacted
    bool redact_network_info{true};  // Whether network information is redacted
    bool redact_memory_contents{true};  // Whether memory contents are redacted
    bool redact_clipboard_data{true};  // Whether clipboard data is redacted
    std::string redaction_level;  // Redaction level (none, minimal, standard, maximum)
    bool fail_closed_on_redaction_failure{true};  // Whether to fail closed if redaction fails
};

}  // namespace kivo::cinema_engine
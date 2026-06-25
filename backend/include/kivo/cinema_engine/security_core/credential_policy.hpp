#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Controls how credentials are stored, redacted, and injected into network requests.
// All credential policies are fail-closed by default.
struct CredentialPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::string credential_storage;  // "memory_only", "encrypted_file", "system_credential_manager"
    bool redact_credentials_in_logs{true};
    bool redact_credentials_in_inspector{true};
    bool allow_credential_injection{true};
    std::string injection_method;  // "header", "query_param", "basic_auth"
    std::string credential_rotation_policy;  // "never", "session_bound", "time_bound"
};

}  // namespace kivo::cinema_engine

#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Privacy consent record for user consent tracking.
// Captures consent status, scope, and revocation capabilities.
struct PrivacyConsentRecord {
    ContractMetadata metadata;
    std::string record_id;  // Unique record identifier
    std::string user_id;  // User identifier
    bool consent_given{false};  // Whether consent was given
    int64_t consent_timestamp{0};  // When consent was given
    std::string consent_scope;  // Scope of consent (crash_dumps, support_bundles, both)
    std::string consent_version;  // Version of consent policy
    bool can_revoke{true};  // Whether consent can be revoked
};

}  // namespace kivo::cinema_engine
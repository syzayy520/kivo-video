#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Code signing policy.
// Defines rules for signing binaries and validating signatures.
struct CodeSigningPolicy {
    ContractMetadata metadata;
    std::string policy_id;  // Unique policy identifier
    bool signing_required{true};  // Whether code signing is required
    bool certificate_validation{true};  // Whether to validate certificates
    bool timestamp_signing{true};  // Whether to use timestamp signing
    std::vector<std::string> allowed_signers;  // List of allowed signer identities
    bool revocation_check{true};  // Whether to check certificate revocation
};

}  // namespace kivo::cinema_engine
#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Policy configuration for the remote direct play gate.
struct RemoteDirectPlayGatePolicy {
    ContractMetadata metadata;
    std::string policy_id;
    bool block_silent_transcode{true};
    bool redact_credentials_in_logs{true};
    bool block_credential_in_redirect{true};
    bool require_cache_identity_binding{true};
};

}  // namespace kivo::cinema_engine

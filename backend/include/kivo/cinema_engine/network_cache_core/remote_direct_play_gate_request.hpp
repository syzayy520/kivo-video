#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Request for a remote direct play gate check.
struct RemoteDirectPlayGateRequest {
    ContractMetadata metadata;
    std::string request_id;
    std::string source_url;
    bool is_direct_play{false};
    bool has_credential{false};
    std::vector<std::string> redirect_chain;
    std::string cache_identity_id;
};

}  // namespace kivo::cinema_engine

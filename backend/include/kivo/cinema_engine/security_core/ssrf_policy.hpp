#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Protects against Server-Side Request Forgery (SSRF) attacks.
// Blocks private IP ranges, localhost, and cloud metadata endpoints by default.
struct SsrfPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    bool block_private_ips{true};
    bool block_localhost{true};
    bool block_metadata_endpoints{true};
    std::vector<std::string> allowed_hosts;
};

}  // namespace kivo::cinema_engine

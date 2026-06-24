#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Network redirect policy for HTTP range requests.
// Enforces credential stripping on cross-origin redirects and
// limits redirect chain depth to prevent infinite loops.
struct RedirectPolicy {
    ContractMetadata metadata;
    int max_redirects{5};
    bool follow_cross_origin{false};
    bool strip_credentials_on_redirect{true};
    std::vector<std::string> allowed_schemes;
};

}  // namespace kivo::cinema_engine

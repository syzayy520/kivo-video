#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Stable identity for an AList resource across link refreshes.
struct AListDirectLinkIdentity {
    ContractMetadata metadata;
    std::string identity_id;
    std::string file_path;
    std::string content_hash;
    int64_t first_link_generated_at{0};
    int64_t link_refresh_count{0};
    int64_t last_refresh_at{0};
};

}  // namespace kivo::cinema_engine

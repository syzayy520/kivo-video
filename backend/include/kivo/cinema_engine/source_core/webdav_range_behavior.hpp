#pragma once

#include <cstdint>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Detailed range request behavior for a WebDAV server.
struct WebDavRangeBehavior {
    ContractMetadata metadata;
    bool range_header_supported{false};
    bool content_range_present{false};
    bool single_byte_range_only{false};
    int64_t chunk_size_limit{0};
    int status_on_full_content{200};
    int status_on_partial{206};
    int status_on_not_satisfiable{416};
};

}  // namespace kivo::cinema_engine

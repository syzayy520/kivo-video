#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Response with direct play or transcoding URL from Emby.
struct EmbyDirectPlayResponse {
    ContractMetadata metadata;
    std::string request_id;
    std::string direct_play_url;
    std::string direct_play_container;
    std::string transcoding_url;
    std::string transcoding_protocol;
    bool is_direct_play{false};
};

}  // namespace kivo::cinema_engine

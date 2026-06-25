#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Fallback strategy when a WebDAV server doesn't support range requests.
enum class WebDavFallbackStrategy {
    FullDownload,
    SeekToZero,
    Abort
};

// Fallback plan when range playback is unavailable.
struct WebDavRangeFallback {
    ContractMetadata metadata;
    std::string fallback_id;
    std::string original_request_url;
    WebDavFallbackStrategy fallback_strategy{WebDavFallbackStrategy::Abort};
    int64_t estimated_download_size{0};
    std::string user_visible_hint;
};

}  // namespace kivo::cinema_engine

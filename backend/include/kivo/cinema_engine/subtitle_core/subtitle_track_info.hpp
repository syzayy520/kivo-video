#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Metadata for a subtitle track.
struct SubtitleTrackInfo {
    ContractMetadata metadata;
    std::string track_id;
    std::string file_path;
    std::string format;
    std::string language;
    bool is_default{false};
    int64_t entry_count{0};
};

}  // namespace kivo::cinema_engine

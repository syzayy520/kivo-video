#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/subtitle_core/subtitle_track_info.hpp"
#include "kivo/cinema_engine/subtitle_core/subtitle_entry.hpp"

namespace kivo::cinema_engine {

// Result of loading a subtitle file.
struct SubtitleLoadResult {
    ContractMetadata metadata;
    bool success{false};
    SubtitleTrackInfo track_info;
    std::vector<SubtitleEntry> entries;
    std::string error_message;
};

}  // namespace kivo::cinema_engine

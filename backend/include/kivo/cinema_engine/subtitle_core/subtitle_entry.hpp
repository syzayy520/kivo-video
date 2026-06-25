#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Individual subtitle entry with timing.
struct SubtitleEntry {
    ContractMetadata metadata;
    int64_t entry_id{0};
    int64_t start_time_ms{0};
    int64_t end_time_ms{0};
    std::string text;
    std::string style;  // ASS style name (empty for SRT)
};

}  // namespace kivo::cinema_engine

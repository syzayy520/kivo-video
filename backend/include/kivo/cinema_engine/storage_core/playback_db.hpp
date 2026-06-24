#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// PlaybackDb is the contract for the playback progress database.
struct PlaybackDb {
    ContractMetadata metadata;
    std::string db_path;
    bool is_open{false};
    bool is_healthy{false};
    std::string corruption_recovery_policy;
    int64_t session_count{0};
    int64_t progress_count{0};
};

}  // namespace kivo::cinema_engine

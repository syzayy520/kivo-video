#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// DemuxRuntime represents the runtime state of a demuxer.
// The actual FFmpeg types stay inside the adapter; only the contract crosses.
struct DemuxRuntime {
    ContractMetadata metadata;
    std::string demux_id;
    std::string source_id;
    std::string container_format;
    bool is_open{false};
    bool is_eof{false};
    int64_t current_position{0};
    int64_t duration{-1};  // in timebase units
    int32_t timebase_num{1};
    int32_t timebase_den{1};
};

}  // namespace kivo::cinema_engine

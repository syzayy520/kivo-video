#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// TimebaseNormalization converts between container timebase, stream timebase,
// and KivoMediaTime (microseconds). All timestamps are normalized to KivoMediaTime.
struct TimebaseNormalization {
    ContractMetadata metadata;
    std::string normalization_id;
    int32_t container_timebase_num{1};
    int32_t container_timebase_den{1};
    int32_t stream_timebase_num{1};
    int32_t stream_timebase_den{1};
    int64_t kivo_media_time_us{0};  // normalized time in microseconds
    bool is_valid{false};
};

}  // namespace kivo::cinema_engine

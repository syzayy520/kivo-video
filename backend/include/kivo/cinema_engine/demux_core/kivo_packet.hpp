#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// KivoPacket is a typed packet wrapper carrying compressed codec data.
struct KivoPacket {
    ContractMetadata metadata;
    std::string packet_id;
    int32_t stream_index{-1};
    int64_t pts{0};         // presentation timestamp in stream timebase
    int64_t dts{0};         // decode timestamp in stream timebase
    int64_t duration{0};
    bool is_key_frame{false};
    bool is_corrupt{false};
    std::vector<uint8_t> data;  // owned compressed packet payload
};

}  // namespace kivo::cinema_engine

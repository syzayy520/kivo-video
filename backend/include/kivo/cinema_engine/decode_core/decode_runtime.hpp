#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// DecodeRuntime represents the runtime state of a decoder.
// Actual codec context stays inside the adapter boundary.
struct DecodeRuntime {
    ContractMetadata metadata;
    std::string decode_id;
    std::string codec_name;
    std::string decode_backend;  // "software", "hardware", "hardware_fallback"
    bool is_open{false};
    bool is_flushed{false};
    int32_t width{0};
    int32_t height{0};
    std::string pixel_format;
    int32_t timebase_num{1};
    int32_t timebase_den{1};
};

}  // namespace kivo::cinema_engine

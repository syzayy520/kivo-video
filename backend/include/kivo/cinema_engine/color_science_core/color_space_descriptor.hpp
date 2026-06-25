#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Color space information for rendering output.
struct ColorSpaceDescriptor {
    ContractMetadata metadata;
    std::string descriptor_id;
    std::string color_space;
    std::string transfer_function;  // pq, hlg, srgb
    std::string primaries;          // bt709, bt2020
    bool is_hdr{false};
    int64_t max_cll{0};  // Max Content Light Level (nits)
    int64_t max_fall{0}; // Max Frame Average Light Level (nits)
};

}  // namespace kivo::cinema_engine

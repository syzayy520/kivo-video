// downmix_coefficient_table.hpp — P6A mix: downmix coefficient table (§9.10)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct DownmixCoefficientTable {
    ContractMetadata metadata;
    double center_gain{0.70710678};
    double surround_gain{0.70710678};
    bool lfe_excluded_default{true};
    bool limiter_required_if_peak_exceeds_0dbfs{true};
    bool operator==(const DownmixCoefficientTable&) const = default;
};
}  // namespace kivo::video::audio_plane::mix

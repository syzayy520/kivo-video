// audio_valid_bits.hpp — P6A format: audio valid bits value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioValidBits {
    int32_t value{0};
    bool operator==(const AudioValidBits&) const = default;
};
}  // namespace kivo::video::audio_plane::format

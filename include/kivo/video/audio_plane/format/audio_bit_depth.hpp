// audio_bit_depth.hpp — P6A format: audio bit depth value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioBitDepth {
    int32_t container_bits{0};
    bool operator==(const AudioBitDepth&) const = default;
};
}  // namespace kivo::video::audio_plane::format

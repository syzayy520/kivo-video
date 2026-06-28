// audio_format_hash.hpp — P6A format: audio format hash value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioFormatHash {
    uint64_t value{0};
    bool is_accelerator_not_sole_truth{true};
    bool operator==(const AudioFormatHash&) const = default;
};
}  // namespace kivo::video::audio_plane::format

// audio_decode_order_index.hpp — P6A time: AudioDecodeOrderIndex (value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioDecodeOrderIndex {
    uint64_t value{0};
    bool operator==(const AudioDecodeOrderIndex&) const = default;
};
}  // namespace kivo::video::audio_plane::time

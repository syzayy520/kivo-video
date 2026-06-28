// audio_codec_id.hpp — P6A format: audio codec id value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioCodecId {
    int32_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioCodecId&) const = default;
};
}  // namespace kivo::video::audio_plane::format

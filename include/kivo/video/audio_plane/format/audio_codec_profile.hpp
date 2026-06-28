// audio_codec_profile.hpp — P6A format: audio codec profile value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioCodecProfile {
    int32_t value{0};
    bool operator==(const AudioCodecProfile&) const = default;
};
}  // namespace kivo::video::audio_plane::format

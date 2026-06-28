// audio_frame_id.hpp — P6A frame: AudioFrameId (§9.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::frame {
struct AudioFrameId {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioFrameId&) const = default;
};
}  // namespace kivo::video::audio_plane::frame

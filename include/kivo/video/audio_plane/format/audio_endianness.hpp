// audio_endianness.hpp — P6A format: audio endianness enum (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
enum class AudioEndianness {
    Little = 0,
    Big = 1
};
}  // namespace kivo::video::audio_plane::format

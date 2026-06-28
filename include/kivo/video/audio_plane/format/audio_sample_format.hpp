// audio_sample_format.hpp — P6A format: audio sample format enum (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
enum class AudioSampleFormat {
    Unknown = 0,
    Float32 = 1,
    Int16 = 2,
    Int24 = 3,
    Int32 = 4,
    S24In32 = 5,
    S24Packed = 6,
    UInt8 = 7
};
}  // namespace kivo::video::audio_plane::format

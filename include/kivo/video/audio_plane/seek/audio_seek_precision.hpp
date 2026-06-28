// audio_seek_precision.hpp — P6A seek: audio seek precision enum (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::seek {
enum class AudioSeekPrecision {
    FastPacketApproximate = 0,
    TimestampAccurate = 1,
    SampleAccurate = 2,
    GaplessPreservingAccurate = 3
};
}  // namespace kivo::video::audio_plane::seek

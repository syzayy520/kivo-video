// audio_format_descriptor.hpp — P6A format: audio format descriptor contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/format/audio_sample_format.hpp"
#include "kivo/video/audio_plane/format/audio_sample_rate_hz.hpp"
#include "kivo/video/audio_plane/format/audio_endianness.hpp"
namespace kivo::video::audio_plane::format {
struct AudioFormatDescriptor {
    kivo::video::audio_plane::ContractMetadata metadata;
    AudioSampleFormat sample_format{AudioSampleFormat::Unknown};
    AudioSampleRateHz sample_rate;
    int32_t channel_count{0};
    int32_t bit_depth{0};
    int32_t valid_bits{0};
    AudioEndianness endianness{AudioEndianness::Little};
};
}  // namespace kivo::video::audio_plane::format

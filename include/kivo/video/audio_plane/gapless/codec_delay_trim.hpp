// codec_delay_trim.hpp — P6A gapless: codec delay trim contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct CodecDelayTrim {
    kivo::video::audio_plane::ContractMetadata metadata;
    int32_t delay_samples{0};
    bool trim_applied{false};
};
}  // namespace kivo::video::audio_plane::gapless

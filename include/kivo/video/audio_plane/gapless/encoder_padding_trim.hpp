// encoder_padding_trim.hpp — P6A gapless: encoder padding trim contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct EncoderPaddingTrim {
    kivo::video::audio_plane::ContractMetadata metadata;
    int32_t padding_samples{0};
    bool trim_applied{false};
};
}  // namespace kivo::video::audio_plane::gapless

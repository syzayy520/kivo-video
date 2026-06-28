// audio_sample_converter_contract.hpp — P6A conversion: sample converter contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
struct AudioSampleConverterContract {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool noexcept_conversion{true};
};
}  // namespace kivo::video::audio_plane::conversion

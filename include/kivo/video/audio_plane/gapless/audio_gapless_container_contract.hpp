// audio_gapless_container_contract.hpp — P6A gapless: container contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct AudioGaplessContainerContract {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool codec_delay_trim_required{false};
    bool encoder_padding_trim_required{false};
};
}  // namespace kivo::video::audio_plane::gapless

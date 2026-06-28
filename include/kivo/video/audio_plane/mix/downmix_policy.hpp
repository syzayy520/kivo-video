// downmix_policy.hpp — P6A mix: downmix policy (§9.9)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
enum class DownmixMode {
    LoRo = 0,
    LtRt = 1,
    StereoFoldDown = 2,
    MonoFoldDown = 3,
    CustomMatrix = 4
};
struct DownmixPolicy {
    ContractMetadata metadata;
    DownmixMode mode{DownmixMode::LoRo};
    bool operator==(const DownmixPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix

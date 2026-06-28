// audio_gain_stage_order.hpp — P6A mix: gain stage ordering (§9.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
enum class GainStage {
    DRC = 0,
    DialogEnhancement = 1,
    PrimarySecondaryMix = 2,
    ReplayGain = 3,
    UserVolume = 4,
    PreLimiter = 5,
    FinalSafetyLimiter = 6
};
}  // namespace kivo::video::audio_plane::mix

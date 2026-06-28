// audio_canonical_speaker_order.hpp — P6A channel: canonical speaker order (§8.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
enum class CanonicalSpeaker {
    FL = 0,
    FR = 1,
    FC = 2,
    LFE = 3,
    BL = 4,
    BR = 5,
    SL = 6,
    SR = 7,
    FLC = 8,
    FRC = 9,
    TFL = 10,
    TFR = 11,
    TFC = 12,
    TBL = 13,
    TBR = 14,
    TBC = 15,
    Unknown = 16
};
}  // namespace kivo::video::audio_plane::channel

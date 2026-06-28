// playback_rate_audio_decision.hpp — P6A rate: playback rate audio decision (§10.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct PlaybackRateAudioDecision {
    ContractMetadata metadata;
    double effective_rate{1.0};
    bool bit_perfect_disabled{true};
    bool passthrough_rejected{true};
    bool operator==(const PlaybackRateAudioDecision&) const = default;
};
}  // namespace kivo::video::audio_plane::rate

// audio_gapless_commit_transition_command.hpp — P6A gapless: commit transition command contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct AudioGaplessCommitTransitionCommand {
    kivo::video::audio_plane::ContractMetadata metadata;
    uint64_t transition_id{0};
    int64_t transition_media_time_ms{0};
};
}  // namespace kivo::video::audio_plane::gapless

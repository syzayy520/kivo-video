// audio_gapless_cancel_reserve_command.hpp — P6A gapless: cancel reserve command contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct AudioGaplessCancelReserveCommand {
    kivo::video::audio_plane::ContractMetadata metadata;
    uint64_t next_track_id{0};
    int32_t cancel_reason{0};
};
}  // namespace kivo::video::audio_plane::gapless

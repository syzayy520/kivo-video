// audio_gapless_prepare_next_command.hpp — P6A gapless: prepare next command contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct AudioGaplessPrepareNextCommand {
    kivo::video::audio_plane::ContractMetadata metadata;
    uint64_t current_track_id{0};
    uint64_t next_track_id{0};
    uint64_t physical_continuity_group_id{0};
    int64_t expected_transition_media_time_ms{0};
    int32_t reserve_policy{0};
    uint64_t codec_private_data_ref{0};
    bool p5_next_packet_boundary_available{false};
};
}  // namespace kivo::video::audio_plane::gapless

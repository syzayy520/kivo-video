// audio_pool_slot_handle.hpp — P6A §8.3: pool slot ownership
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
namespace kivo::video::audio_plane::data {
struct AudioPoolSlotHandle {
    ContractMetadata metadata;
    AudioPoolId pool_id;
    AudioPoolSlotId slot_id;
    AudioAllocationEpoch allocation_epoch;
    packet::AudioPacketId packet_id;
    // Rules: lightweight retire may be wait-free only if proven; heavy retire → non-realtime
    static constexpr bool lightweight_retire_wait_free_if_proven{true};
    static constexpr bool heavy_retire_to_non_realtime{true};
};
}  // namespace kivo::video::audio_plane::data

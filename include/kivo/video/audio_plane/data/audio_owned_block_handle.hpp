// audio_owned_block_handle.hpp — P6A §8.2: move-owned block
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
namespace kivo::video::audio_plane::data {
struct AudioDataHandleFlags { bool is_heavy_release{false}; bool is_noexcept_release{true}; bool operator==(const AudioDataHandleFlags&) const = default; };
using AudioOwnedBlockReleaseFn = void(*)(void* ctx) noexcept;
struct AudioOwnedBlockHandle {
    ContractMetadata metadata;
    AudioDataHandleId handle_id;
    AudioAllocationEpoch allocation_epoch;
    packet::AudioPacketId packet_id;
    AudioOwnedBlockReleaseFn release_fn{nullptr};
    void* release_ctx{nullptr};
    AudioDataHandleFlags flags;
    // Rules: P6 owns release after AcceptedMoved; release_fn must be noexcept
    static constexpr bool release_fn_must_be_noexcept{true};
    static constexpr bool heavy_release_not_on_device_thread{true};
};
}  // namespace kivo::video::audio_plane::data

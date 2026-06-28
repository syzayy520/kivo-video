// audio_sync_byte_view.hpp — P6A §8.1: synchronous-call-only borrowed view
#pragma once
#include <cstdint>
#include <span>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
namespace kivo::video::audio_plane::data {
enum class AudioDataOwnerStage : int32_t { P5 = 0, P6 = 1, Backend = 2, Released = 3 };
struct AudioSyncByteView {
    ContractMetadata metadata;
    AudioDataHandleId handle_id;
    std::span<const std::byte> bytes;
    AudioDataOwnerStage owner_stage{AudioDataOwnerStage::P5};
    packet::AudioPacketId packet_id;
    // Rules: sync-call-only, must not be stored, must not cross thread
    static constexpr bool sync_call_only{true};
    static constexpr bool must_not_be_stored{true};
    static constexpr bool must_not_cross_thread{true};
};
}  // namespace kivo::video::audio_plane::data

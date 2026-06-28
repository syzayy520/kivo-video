// audio_spsc_memory_order.hpp — P6A §15: memory order specification
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
struct AudioSpscMemoryOrder {
    ContractMetadata metadata;
    bool producer_publish_uses_release{true};
    bool consumer_read_uses_acquire{true};
    bool diagnostic_counters_relaxed{true};
    bool producer_writes_payload_before_publishing_slot{true};
};
}  // namespace kivo::video::audio_plane::render

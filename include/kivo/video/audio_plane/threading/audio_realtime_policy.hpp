// audio_realtime_policy.hpp — P6A threading: realtime policy (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioRealtimePolicy
// Policy for realtime audio threads. Compile-only skeleton.
struct AudioRealtimePolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool no_throw{true};
    bool no_production_assert{true};
    bool no_heap_alloc{true};
    bool no_block_io{true};

    bool operator==(const AudioRealtimePolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

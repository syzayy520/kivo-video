// audio_render_backpressure_protocol.hpp — P6A §9/§17: backpressure
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
enum class BackpressureMode { Normal=0, Live=1, BitPerfect=2, Passthrough=3 };
struct AudioRenderBackpressureProtocol {
    ContractMetadata metadata;
    bool normal_backpressure_upstream{true};
    bool live_drop_according_to_policy{true};
    bool bit_perfect_rebuffer_or_fail{true};
    bool passthrough_drop_only_complete_stale_burst{true};
    bool decode_pause_canceled_when_underfill_starts{true};
    bool control_thread_owns_buffering_recovering_transition{true};
};
}  // namespace kivo::video::audio_plane::render

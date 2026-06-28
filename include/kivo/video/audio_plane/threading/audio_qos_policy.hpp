// audio_qos_policy.hpp — P6A threading: QoS policy (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioQosPolicy
// Quality of service policy for audio vs video. Compile-only skeleton.
struct AudioQosPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool audio_priority_over_video{true};
    bool emit_qos_degraded_event{true};
    bool p6_cannot_command_p5_frame_drop{true};

    bool operator==(const AudioQosPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

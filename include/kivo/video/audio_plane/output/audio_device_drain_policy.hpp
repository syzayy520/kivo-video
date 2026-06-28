// audio_device_drain_policy.hpp — P6A §11: drain semantics
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
struct AudioDeviceDrainPolicy {
    ContractMetadata metadata;
    int32_t drain_timeout_ms{3000};
    bool cancel_on_new_command{true};
    bool emit_drain_complete_evidence{true};
    bool supersede_after_drain{false};
};
}  // namespace kivo::video::audio_plane::output

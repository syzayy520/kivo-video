// audio_command_deferred_policy.hpp — P6A command: AudioCommandDeferredPolicy (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
struct AudioCommandDeferredPolicy {
    ContractMetadata metadata;
    bool deferred_allowed{true};
    int32_t max_deferred_count{8};
    int64_t max_deferred_duration_ms{5000};
};
}  // namespace kivo::video::audio_plane::command

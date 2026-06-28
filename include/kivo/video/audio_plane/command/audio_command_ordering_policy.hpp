// audio_command_ordering_policy.hpp — P6A command: AudioCommandOrderingPolicy (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
struct AudioCommandOrderingPolicy {
    ContractMetadata metadata;
    bool seek_supersedes_older_seek{true};
    bool close_supersedes_stream_local{true};
    bool track_switch_supersedes_old_track_decode{true};
    bool device_reset_concurrent_via_recovery_coordinator{true};
};
}  // namespace kivo::video::audio_plane::command

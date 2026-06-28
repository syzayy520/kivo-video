// audio_command_idempotency_policy.hpp — P6A command: AudioCommandIdempotencyPolicy (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
struct AudioCommandIdempotencyPolicy {
    ContractMetadata metadata;
    bool duplicate_before_terminal_returns_snapshot{true};
    bool duplicate_after_terminal_returns_same_ack{true};
    bool same_payload_different_id_is_new{true};
    bool retry_marker_required_for_duplicate{true};
};
}  // namespace kivo::video::audio_plane::command

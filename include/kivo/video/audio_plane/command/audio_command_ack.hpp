// audio_command_ack.hpp — P6A command: AudioCommandAck terminal acknowledgement (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
enum class AudioCommandTerminalAck {
    Executed,
    Failed,
    Superseded,
    Rejected
};
struct AudioCommandAck {
    ContractMetadata metadata;
    AudioCommandTerminalAck ack{AudioCommandTerminalAck::Executed};
    uint64_t command_id{0};
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::command

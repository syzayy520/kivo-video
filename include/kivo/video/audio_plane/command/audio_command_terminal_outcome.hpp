// audio_command_terminal_outcome.hpp — P6A command: AudioCommandTerminalOutcome (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/command/audio_command_ack.hpp"
namespace kivo::video::audio_plane::command {
struct AudioCommandTerminalOutcome {
    ContractMetadata metadata;
    AudioCommandTerminalAck ack{AudioCommandTerminalAck::Executed};
    uint64_t command_id{0};
    bool exactly_one_terminal{true};
};
}  // namespace kivo::video::audio_plane::command

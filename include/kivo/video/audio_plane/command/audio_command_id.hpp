// audio_command_id.hpp — P6A command: AudioCommandId value type (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
struct AudioCommandId {
    uint64_t value{0};
    bool operator==(const AudioCommandId&) const = default;
    bool is_valid() const { return value != 0; }
};
}  // namespace kivo::video::audio_plane::command

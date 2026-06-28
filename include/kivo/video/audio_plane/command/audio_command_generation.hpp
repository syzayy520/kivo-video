// audio_command_generation.hpp — P6A command: AudioCommandGeneration value type (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
struct AudioCommandGeneration {
    uint64_t value{0};
    bool operator==(const AudioCommandGeneration&) const = default;
    bool is_valid() const { return value != 0; }
};
}  // namespace kivo::video::audio_plane::command

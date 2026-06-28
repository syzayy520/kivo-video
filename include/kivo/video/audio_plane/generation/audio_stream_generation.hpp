// audio_stream_generation.hpp — P6A §13: stream generation
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::generation {
struct AudioStreamGeneration {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioStreamGeneration&) const = default;
};
}  // namespace kivo::video::audio_plane::generation

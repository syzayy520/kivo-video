// audio_format_generation.hpp — P6A §13: format generation
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::generation {
struct AudioFormatGeneration {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioFormatGeneration&) const = default;
};
}  // namespace kivo::video::audio_plane::generation

// audio_device_generation.hpp — P6A §13: device generation
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::generation {
struct AudioDeviceGeneration {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioDeviceGeneration&) const = default;
};
}  // namespace kivo::video::audio_plane::generation

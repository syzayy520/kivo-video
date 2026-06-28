// audio_engine_epoch.hpp — P6A §13: engine epoch (0=invalid)
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::generation {
struct AudioEngineEpoch {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioEngineEpoch&) const = default;
};
}  // namespace kivo::video::audio_plane::generation

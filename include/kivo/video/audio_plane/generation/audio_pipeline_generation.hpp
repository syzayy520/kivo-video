// audio_pipeline_generation.hpp — P6A §13: pipeline generation
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::generation {
struct AudioPipelineGeneration {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioPipelineGeneration&) const = default;
};
}  // namespace kivo::video::audio_plane::generation

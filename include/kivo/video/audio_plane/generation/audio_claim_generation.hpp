// audio_claim_generation.hpp — P6A §13: claim generation
#pragma once
#include <cstdint>
namespace kivo::video::audio_plane::generation {
struct AudioClaimGeneration {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const AudioClaimGeneration&) const = default;
};
}  // namespace kivo::video::audio_plane::generation

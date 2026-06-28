// audio_denormal_float_policy.hpp — P6A threading: denormal float (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioDenormalFloatPolicy
// Policy for denormal float handling. Compile-only skeleton.
struct AudioDenormalFloatPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool ftz_enabled{true};
    bool daz_enabled{true};
    bool integer_bit_perfect_unaffected{true};

    bool operator==(const AudioDenormalFloatPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

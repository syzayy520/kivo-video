// audio_mmcss_profile.hpp — P6A threading: MMCSS profile (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioMmcssProfile
// MMCSS task profile for an audio thread. Compile-only skeleton.
struct AudioMmcssProfile {
    kivo::video::audio_plane::ContractMetadata metadata{};
    const char* profile_name{"Audio"};
    int32_t priority{0};

    bool operator==(const AudioMmcssProfile&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

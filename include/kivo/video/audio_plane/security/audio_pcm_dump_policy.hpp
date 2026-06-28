// audio_pcm_dump_policy.hpp — P6A security: PCM dump policy (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AudioPcmDumpPolicy
// PCM dump restrictions. Compile-only skeleton.
struct AudioPcmDumpPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool protected_pcm_dump_forbidden{true};
    bool normal_pcm_dump_forbidden_by_default{true};
    bool normal_requires_explicit_diagnostic_mode{true};
    bool normal_requires_user_approval{true};
    bool normal_must_be_bounded_duration{true};
    bool normal_must_exclude_protected{true};

    bool operator==(const AudioPcmDumpPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::security

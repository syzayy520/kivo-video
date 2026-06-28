// audio_format_hash_algorithm.hpp — P6A format: audio format hash algorithm policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
enum class AudioFormatHashAlgorithm {
    XxHash3_64 = 0,
    Fnv1a_64 = 1
};
struct AudioFormatHashAlgorithmPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    AudioFormatHashAlgorithm preferred{AudioFormatHashAlgorithm::XxHash3_64};
    bool collision_check_required{true};
};
}  // namespace kivo::video::audio_plane::format

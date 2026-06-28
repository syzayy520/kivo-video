// audio_accurate_seek_policy.hpp — P6A seek: accurate seek policy contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::seek {
struct AudioAccurateSeekPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool decodes_from_preroll{true};
    bool discards_until_target_timestamp{true};
};
}  // namespace kivo::video::audio_plane::seek

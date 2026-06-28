// audio_bilingual_term_map.hpp — P6A terminology: bilingual map (§terminology)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::terminology {

// AudioBilingualTermMap
// Bilingual mapping of audio terms. Compile-only skeleton.
struct AudioBilingualTermMap {
    kivo::video::audio_plane::ContractMetadata metadata{};
    const char* en_bit_perfect{"bit_perfect"};
    const char* zh_bit_perfect{"\xe4\xbd\x8d\xe5\xae\x8c\xe7\xbe\x8e"};

    bool operator==(const AudioBilingualTermMap&) const = default;
};

}  // namespace kivo::video::audio_plane::terminology

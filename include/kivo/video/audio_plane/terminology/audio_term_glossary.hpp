// audio_term_glossary.hpp — P6A terminology: glossary (§terminology)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::terminology {

// AudioTermGlossary
// Canonical glossary of audio terms. Compile-only skeleton.
struct AudioTermGlossary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    const char* bit_perfect{"exact PCM reproduction, no mutating stages"};
    const char* passthrough{"encoded audio delivered to endpoint"};
    const char* gapless{"sample-continuous track transition"};

    bool operator==(const AudioTermGlossary&) const = default;
};

}  // namespace kivo::video::audio_plane::terminology

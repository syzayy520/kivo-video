// audio_dither_policy.hpp — P6A conversion: dither policy (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::conversion {
enum class DitherKind {
    None = 0,
    Triangular = 1,
    Rectangular = 2
};
struct AudioDitherPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    DitherKind kind{DitherKind::None};
    bool applied_before_quantization{true};
};
}  // namespace kivo::video::audio_plane::conversion

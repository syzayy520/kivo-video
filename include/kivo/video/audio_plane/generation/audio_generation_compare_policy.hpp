// audio_generation_compare_policy.hpp — P6A §13: equality-only comparison
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::generation {
struct AudioGenerationComparePolicy {
    ContractMetadata metadata;
    bool cross_epoch_comparison_invalid{true};
    bool equality_only_for_freshness{true};
    bool less_than_only_within_same_epoch{true};
    bool forbidden_less_equal_means_fresh{true};
};
}  // namespace kivo::video::audio_plane::generation

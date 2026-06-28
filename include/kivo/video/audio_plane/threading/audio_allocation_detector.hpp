// audio_allocation_detector.hpp — P6A threading: allocation detector (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioAllocationDetector
// Detector for heap allocations on realtime threads. Compile-only skeleton.
struct AudioAllocationDetector {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool detect_heap_alloc{true};
    bool detect_operator_new{true};
    int32_t hot_path_alloc_count{0};

    bool operator==(const AudioAllocationDetector&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

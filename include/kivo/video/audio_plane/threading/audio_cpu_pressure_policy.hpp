// audio_cpu_pressure_policy.hpp — P6A threading: CPU pressure (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioCpuPressurePolicy
// Policy for CPU pressure handling. Compile-only skeleton.
struct AudioCpuPressurePolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool audio_device_thread_priority{true};
    bool p4_may_reduce_video_quality{true};

    bool operator==(const AudioCpuPressurePolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

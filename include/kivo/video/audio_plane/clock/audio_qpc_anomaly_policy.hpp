// audio_qpc_anomaly_policy.hpp — P6A clock: AudioQpcAnomalyPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
enum class QpcAnomalyKind {
    BackwardJump = 0,
    ImpossibleForwardJump = 1,
    SuspendResume = 2,
    VmUnstable = 3,
};
struct AudioQpcAnomalyPolicy {
    ContractMetadata metadata;
    bool backward_jump_invalidate{true};
    bool impossible_forward_jump_threshold_5s_or_100_periods{true};
    bool suspend_resume_invalidate_and_rebuild{true};
    bool vm_unstable_increase_error_bound{true};
    bool operator==(const AudioQpcAnomalyPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::clock

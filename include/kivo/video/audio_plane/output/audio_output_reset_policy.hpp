// audio_output_reset_policy.hpp — P6A §40: device reset policy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
enum class ResetReason : int32_t { UserRequest = 0, DeviceLost = 1, FormatChanged = 2, Recovery = 3, PowerResume = 4 };
struct AudioOutputResetPolicy {
    ContractMetadata metadata;
    bool invalidate_clock_snapshots{true};
    bool invalidate_qpc_mapping{true};
    bool drop_stale_by_generation{true};
    bool request_recovery_seek{true};
    int32_t max_recovery_attempts{3};
};
}  // namespace kivo::video::audio_plane::output

// audio_generation_wrap_policy.hpp — P6A §13: wrap detection
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::generation {
enum class WrapAction : int32_t { NoWrap = 0, EngineEpochResetRequired = 1, StopActivePipelines = 2, InvalidateAllSnapshots = 3 };
struct AudioGenerationWrapPolicy {
    ContractMetadata metadata;
    bool normal_runtime_wrap_unreachable{true};
    bool increment_to_zero_triggers_epoch_reset{true};
    bool stop_active_pipelines_on_wrap{true};
    bool invalidate_all_snapshots_on_wrap{true};
    bool reset_counters_under_new_epoch{true};
};
}  // namespace kivo::video::audio_plane::generation

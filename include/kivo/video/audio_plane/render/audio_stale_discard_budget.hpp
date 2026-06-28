// audio_stale_discard_budget.hpp — P6A §15: stale discard budget
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
struct AudioStaleDiscardBudget {
    ContractMetadata metadata;
    int32_t max_stale_discards_per_event{64};
    int64_t max_event_cpu_budget_ns{0};  // min(25% of device period, policy cap)
    bool stop_discard_loop_if_exceeded{true};
    bool set_async_recovery_flag_if_exceeded{true};
    bool emit_coalesced_evidence_if_exceeded{true};
};
}  // namespace kivo::video::audio_plane::render

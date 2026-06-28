// av_sync_decision.hpp — P6A sync: AvSyncDecision
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
enum class AvSyncDecisionKind {
    NoAction = 0,
    WaitForAudio = 1,
    AdvanceAudio = 2,
    Rebuffer = 3,
};
struct AvSyncDecision {
    ContractMetadata metadata;
    AvSyncDecisionKind kind{AvSyncDecisionKind::NoAction};
    uint64_t p4_decision_id{0};
    bool operator==(const AvSyncDecision&) const = default;
};
}  // namespace kivo::video::audio_plane::sync

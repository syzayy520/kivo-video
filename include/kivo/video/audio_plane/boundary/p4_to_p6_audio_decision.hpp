// p4_to_p6_audio_decision.hpp — P6A boundary: P4→P6 audio decision (§6, §7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P4ToP6AudioDecision
// Decision returned across the P4 (orchestration) ← P6 (audio media plane)
// boundary, correlating to a previously issued P4ToP6AudioCommand via
// correlation_command_id. decision_kind is an enum-as-int placeholder.
struct P4ToP6AudioDecision {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t decision_id{0};
    uint64_t correlation_command_id{0};
    int32_t decision_kind{0};
    bool authorized{false};

    bool operator==(const P4ToP6AudioDecision&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary

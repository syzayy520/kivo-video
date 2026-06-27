// epoch_condition.hpp — GATE-022: P5A presentation epoch condition
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::presentation {

enum class EpochCondition {
    Initializing,                           // epoch is initializing
    Active,                                 // epoch is active
    Suspended,                              // epoch is suspended
    Ended                                   // epoch has ended
};

struct PresentationEpoch {
    EpochCondition condition{EpochCondition::Initializing};
    uint64_t epoch_id{0};                   // unique epoch identifier
    uint64_t start_timestamp{0};            // epoch start timestamp
    bool operator==(PresentationEpoch const&) const = default;
};

}  // namespace kivo::video_plane::presentation

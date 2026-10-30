#include "video/video_plane/core/pace/pace_action.hpp"

namespace kivo::video::core::pace {

const char* pace_action_name(PaceAction action) noexcept {
    switch (action) {
        case PaceAction::kWait:
            return "wait";
        case PaceAction::kPresent:
            return "present";
        case PaceAction::kDrop:
            return "drop";
        case PaceAction::kCatchUp:
            return "catch_up";
    }
    return "unknown";
}

const char* clock_health_name(ClockHealth health) noexcept {
    switch (health) {
        case ClockHealth::kFresh:
            return "fresh";
        case ClockHealth::kExtrapolated:
            return "extrapolated";
        case ClockHealth::kFreeRun:
            return "free_run";
    }
    return "unknown";
}

}  // namespace kivo::video::core::pace

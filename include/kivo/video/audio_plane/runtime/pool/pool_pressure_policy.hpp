// pool_pressure_policy.hpp — P6B pool: pressure-driven action policy
// HEADER-ONLY. Maps (playback mode, pressure) to a pressure action.
#pragma once
#include <cstdint>

namespace kivo::video::audio_plane::runtime::pool {

// PlaybackMode: the mode governing pressure tolerance.
enum class PlaybackMode : int32_t {
    Normal = 0,
    Live = 1,
    Bitperfect = 2,
    Passthrough = 3,
};

// PressureAction: the action taken when pool pressure crosses a threshold.
enum class PressureAction : int32_t {
    Backpressure = 0,  // signal upstream to slow down
    Drop = 1,          // discard a packet/frame
    Rebuffer = 2,      // pause and rebuffer (never silent drop)
    Fail = 3,          // hard failure
};

// PoolPressurePolicy
// Stateless policy deciding the action for a given playback mode and pressure.
// Pressure is a normalized double in [0.0, 1.0].
//
// Mode-specific rules:
//   Normal:      Backpressure if pressure > 0.8, else no action (Fail sentinel).
//   Live:        Drop if pressure > 0.9, else no action.
//   Bitperfect:  Rebuffer if pressure > 0.8 (never silent drop).
//   Passthrough: Drop only complete stale unit if pressure > 0.95.
class PoolPressurePolicy {
public:
    PoolPressurePolicy() = default;

    // Decide the action for the given mode and pressure.
    // Returns Backpressure as the "no critical action" default for pressure
    // below threshold in Normal mode; callers may treat Backpressure at low
    // pressure as a no-op signal.
    PressureAction decide(PlaybackMode mode, double pressure) const noexcept {
        switch (mode) {
            case PlaybackMode::Normal:
                if (pressure > 0.8) {
                    return PressureAction::Backpressure;
                }
                return PressureAction::Backpressure;  // low pressure: gentle signal
            case PlaybackMode::Live:
                if (pressure > 0.9) {
                    return PressureAction::Drop;
                }
                return PressureAction::Backpressure;
            case PlaybackMode::Bitperfect:
                if (pressure > 0.8) {
                    return PressureAction::Rebuffer;  // never silent drop
                }
                return PressureAction::Backpressure;
            case PlaybackMode::Passthrough:
                if (pressure > 0.95) {
                    return PressureAction::Drop;  // only complete stale unit
                }
                return PressureAction::Backpressure;
        }
        return PressureAction::Fail;
    }

    // Convenience: threshold for the given mode.
    static constexpr double threshold(PlaybackMode mode) noexcept {
        switch (mode) {
            case PlaybackMode::Normal:      return 0.8;
            case PlaybackMode::Live:        return 0.9;
            case PlaybackMode::Bitperfect:  return 0.8;
            case PlaybackMode::Passthrough: return 0.95;
        }
        return 0.8;
    }
};

}  // namespace kivo::video::audio_plane::runtime::pool

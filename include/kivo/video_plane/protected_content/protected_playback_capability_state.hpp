// protected_playback_capability_state.hpp — GATE-011: P5A protected playback capability state
#pragma once
#include <string>

namespace kivo::video_plane::protected_content {

enum class ProtectedPlaybackState {
    NotRequired,                            // content does not require protection
    Required,                               // protection required but not yet established
    Established,                            // protection established and active
    Lost,                                   // protection was lost during playback
    Failed                                  // protection failed to establish
};

struct ProtectedPlaybackCapabilityState {
    ProtectedPlaybackState state{ProtectedPlaybackState::NotRequired};
    bool secure_decode_available{false};    // hardware secure decode available
    bool protected_output_available{false}; // protected output available
    std::string failure_reason;             // reason if state is Lost or Failed
    bool operator==(ProtectedPlaybackCapabilityState const&) const = default;
};

}  // namespace kivo::video_plane::protected_content

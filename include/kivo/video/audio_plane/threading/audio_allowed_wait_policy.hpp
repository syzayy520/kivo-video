// audio_allowed_wait_policy.hpp — P6A threading: allowed wait policy (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioAllowedWaitPolicy
// Declares which waits are allowed on which threads. Compile-only skeleton.
struct AudioAllowedWaitPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool device_event{true};  // platform-neutral: WASAPI event on Windows
    bool shutdown_event{true};
    bool bounded_device_recovery_wake{true};
    bool decode_wait_forbidden{true};
    bool control_wait_forbidden{true};
    bool evidence_wait_forbidden{true};
    bool file_wait_forbidden{true};
    bool network_wait_forbidden{true};
    bool ui_wait_forbidden{true};
    bool unbounded_mutex_forbidden{true};
    bool condition_variable_forbidden{true};

    bool operator==(const AudioAllowedWaitPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

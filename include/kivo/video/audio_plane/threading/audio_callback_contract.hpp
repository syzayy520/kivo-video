// audio_callback_contract.hpp — P6A threading: callback contract (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioCallbackContract
// Contract for audio device callbacks. Compile-only skeleton.
struct AudioCallbackContract {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool forward_compact_event_to_control{true};
    bool no_durable_evidence_directly{true};

    bool operator==(const AudioCallbackContract&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

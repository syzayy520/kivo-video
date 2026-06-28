// audio_thread_model.hpp — P6A threading: thread model (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioThreadKind
// Kind of audio thread. Compile-only enum.
enum class AudioThreadKind {
    RenderPrepare = 0,
    Device = 1,
    Decode = 2,
    Control = 3,
    Recovery = 4,
    Evidence = 5,
    DeviceNotification = 6
};

// AudioThreadModel
// Model of an audio thread. Compile-only skeleton.
struct AudioThreadModel {
    kivo::video::audio_plane::ContractMetadata metadata{};
    AudioThreadKind kind{AudioThreadKind::Control};
    bool is_realtime{false};

    bool operator==(const AudioThreadModel&) const = default;
};

}  // namespace kivo::video::audio_plane::threading

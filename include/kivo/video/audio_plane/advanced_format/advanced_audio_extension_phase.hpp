// advanced_audio_extension_phase.hpp — P6A advanced_format: extension phase (§advanced_format)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::advanced_format {

// AdvancedAudioExtensionPhase
// Phase of advanced audio extension support. Compile-only enum.
enum class AdvancedAudioExtensionPhase {
    NotSupported = 0,
    Deferred = 1,
    Future = 2
};

// AdvancedAudioExtensionPhaseGate
// Gate for advanced audio extension phase. Compile-only skeleton.
struct AdvancedAudioExtensionPhaseGate {
    kivo::video::audio_plane::ContractMetadata metadata{};
    AdvancedAudioExtensionPhase phase{AdvancedAudioExtensionPhase::NotSupported};

    bool operator==(const AdvancedAudioExtensionPhaseGate&) const = default;
};

}  // namespace kivo::video::audio_plane::advanced_format

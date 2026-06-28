// audio_platform_extension_boundary.hpp — P6A platform: AudioPlatformExtensionBoundary (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::platform {
enum class AudioPlatformExtensionPhase {
    NotSupported,
    Deferred,
    Future
};
struct AudioPlatformExtensionBoundary {
    ContractMetadata metadata;
    AudioPlatformExtensionPhase phase{AudioPlatformExtensionPhase::NotSupported};
};
}  // namespace kivo::video::audio_plane::platform

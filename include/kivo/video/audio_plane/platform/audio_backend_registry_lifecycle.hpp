// audio_backend_registry_lifecycle.hpp — P6A platform: AudioBackendRegistryLifecycle (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/platform/audio_platform_backend_id.hpp"
namespace kivo::video::audio_plane::platform {
enum class AudioBackendRegistryState {
    NotRegistered,
    Registered,
    Active,
    Deactivated
};
struct AudioBackendRegistryLifecycle {
    ContractMetadata metadata;
    AudioBackendRegistryState state{AudioBackendRegistryState::NotRegistered};
    AudioPlatformBackendId backend_id;
};
}  // namespace kivo::video::audio_plane::platform

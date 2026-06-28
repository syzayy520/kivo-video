// audio_platform_capability.hpp — P6A platform: AudioPlatformCapability (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::platform {
struct AudioPlatformCapability {
    ContractMetadata metadata;
    bool concrete_backend_available{false};  // platform-neutral: WASAPI on Windows
    bool fake_available{true};
    int32_t max_channels{0};
    int32_t max_sample_rate_hz{0};
};
}  // namespace kivo::video::audio_plane::platform

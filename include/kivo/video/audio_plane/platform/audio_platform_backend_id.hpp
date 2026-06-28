// audio_platform_backend_id.hpp — P6A platform: AudioPlatformBackendId value type (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::platform {
struct AudioPlatformBackendId {
    uint32_t value{0};
    bool operator==(const AudioPlatformBackendId&) const = default;
    bool is_valid() const { return value != 0; }
};
}  // namespace kivo::video::audio_plane::platform

// audio_output_backend_contract.hpp — P6A platform: AudioOutputBackendContract (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/platform/audio_platform_backend_id.hpp"
namespace kivo::video::audio_plane::platform {
struct AudioOutputBackendContract {
    ContractMetadata metadata;
    AudioPlatformBackendId backend_id;
    int32_t backend_kind{0};
    bool supports_event_driven{false};
    bool supports_exclusive{false};
};
}  // namespace kivo::video::audio_plane::platform

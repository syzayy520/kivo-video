// audio_device_reprime_policy.hpp — P6A §17/§40: reprime after recovery
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
struct AudioDeviceReprimePolicy {
    ContractMetadata metadata;
    int32_t preroll_ms{200};
    int32_t max_preroll_ms{2000};
    bool claim_stale_until_verified{true};
    bool emit_evidence{true};
};
}  // namespace kivo::video::audio_plane::output

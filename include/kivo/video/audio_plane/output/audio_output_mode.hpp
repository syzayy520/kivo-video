// audio_output_mode.hpp — P6A §19: backend-neutral output mode (NO WasapiMode)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
enum class AudioOutputMode : int32_t {
    Unknown = 0,
    PcmSharedCompatible = 1,
    PcmExclusiveExact = 2,
    PcmExclusiveLowLatency = 3,
    EncodedPassthrough = 4,
    MutedNoDevice = 5,
    DisabledNoAudioTrack = 6,
    Failed = 7,
};
struct AudioOutputModeContract {
    ContractMetadata metadata;
    AudioOutputMode mode{AudioOutputMode::Unknown};
    bool shared_can_claim_bit_perfect{false};
    bool muted_may_keep_p4_timeline_authority{true};
    bool disabled_means_no_audio_clock_authority{true};
};
}  // namespace kivo::video::audio_plane::output

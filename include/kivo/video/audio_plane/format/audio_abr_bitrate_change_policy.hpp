// audio_abr_bitrate_change_policy.hpp — P6A format: audio abr bitrate change policy contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioAbrBitrateChangePolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool no_generation_required_if_8_conditions_met{true};
    bool same_codec_profile_samplerate_channels_format{true};
    bool decoder_no_reinit{true};
    bool codec_private_data_unchanged{true};
    bool timestamp_continuity{true};
    bool priming_delay_unchanged{true};
    bool passthrough_framing_unchanged{true};
    bool no_pending_old_config_frames{true};
    bool evidence_emitted{true};
};
}  // namespace kivo::video::audio_plane::format

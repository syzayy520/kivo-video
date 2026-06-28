// audio_output_capability_snapshot.hpp — P6A §36: 20+ field capability snapshot
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/platform/audio_platform_backend_id.hpp"
namespace kivo::video::audio_plane::output {
enum class VolumeConfidence : int32_t { Unknown = 0, SoftwarePossiblyHeuristic = 1, TrustedEndpointApi = 2, LabVerifiedHardware = 3 };
enum class ApoStatus : int32_t { Unknown = 0, Impossible = 1, Bypassed = 2, Active = 3 };
struct AudioOutputCapabilitySnapshot {
    ContractMetadata metadata;
    platform::AudioPlatformBackendId backend_id;
    int32_t device_role{0};
    int32_t shared_formats_mask{0};
    int32_t exclusive_formats_mask{0};
    int32_t passthrough_formats_mask{0};
    int32_t min_period_ms{0};
    int32_t default_period_ms{0};
    int32_t max_period_ms{0};
    bool event_driven_supported{false};
    bool exclusive_supported{false};
    bool raw_mode_supported{false};
    bool offload_supported{false};
    int32_t channel_masks{0};
    int32_t valid_bits_support{0};
    int32_t s24_alignment_policy{0};
    int32_t volume_path_evidence{0};
    VolumeConfidence endpoint_volume_confidence{VolumeConfidence::Unknown};
    VolumeConfidence session_volume_confidence{VolumeConfidence::Unknown};
    bool ducking_support{false};
    VolumeConfidence ducking_confidence{VolumeConfidence::Unknown};
    ApoStatus apo_status{ApoStatus::Unknown};
    VolumeConfidence apo_confidence{VolumeConfidence::Unknown};
    int32_t transport_hint{0};
    uint64_t capability_generation{0};
    uint64_t evidence_id{0};
    bool unknown_blocks_positive_claim() const {
        return endpoint_volume_confidence == VolumeConfidence::Unknown
            || apo_status == ApoStatus::Unknown
            || ducking_confidence == VolumeConfidence::Unknown;
    }
};
}  // namespace kivo::video::audio_plane::output

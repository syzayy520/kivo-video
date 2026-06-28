// audio_packet_discontinuity.hpp — P6A §7/§14: discontinuity marker
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::packet {
enum class DiscontinuityReason : int32_t { Seek = 0, Flush = 1, TrackSwitch = 2, FormatChange = 3, BackwardTimestamp = 4, DeviceReset = 5 };
struct AudioPacketDiscontinuity {
    ContractMetadata metadata;
    DiscontinuityReason reason{DiscontinuityReason::Seek};
    uint64_t pipeline_generation{0};
    bool requires_evidence{true};
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::packet

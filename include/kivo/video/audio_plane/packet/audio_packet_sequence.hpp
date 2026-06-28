// audio_packet_sequence.hpp — P6A §7: sequence monotonicity
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
namespace kivo::video::audio_plane::packet {
struct AudioPacketSequencePolicy {
    ContractMetadata metadata;
    bool monotonic_unless_discontinuity{true};
    bool duplicate_rejected_unless_retry_marker{true};
    bool backward_timestamp_with_monotonic_seq_is_discontinuity{true};
};
}  // namespace kivo::video::audio_plane::packet

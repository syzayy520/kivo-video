// audio_packet_order_violation.hpp — P6A §7: order violation evidence
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::packet {
enum class PacketOrderViolationKind : int32_t { NonMonotonicWithoutDiscontinuity = 0, DuplicateWithoutRetryMarker = 1, BackwardTimestampWithMonotonicSeq = 2 };
struct AudioPacketOrderViolation {
    ContractMetadata metadata;
    PacketOrderViolationKind kind{PacketOrderViolationKind::NonMonotonicWithoutDiscontinuity};
    uint64_t expected_sequence{0};
    uint64_t actual_sequence{0};
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::packet

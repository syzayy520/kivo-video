// audio_packet_import_evidence.hpp — P6A §32: packet import evidence
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_import_result.hpp"
namespace kivo::video::audio_plane::packet {
struct AudioPacketImportEvidence {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    uint64_t packet_id{0};
    AudioPacketImportResult result{AudioPacketImportResult::RejectedInvalidGeneration};
    int64_t timestamp_ms{0};
};
}  // namespace kivo::video::audio_plane::packet

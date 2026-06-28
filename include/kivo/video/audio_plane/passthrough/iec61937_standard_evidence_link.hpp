// iec61937_standard_evidence_link.hpp — P6A passthrough: Iec61937StandardEvidenceLink (§8.7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
struct Iec61937StandardEvidenceLink {
    ContractMetadata metadata;
    uint64_t standard_ref{0};
    uint64_t evidence_id{0};
    int32_t data_type_mapping{0};
    bool operator==(const Iec61937StandardEvidenceLink&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough

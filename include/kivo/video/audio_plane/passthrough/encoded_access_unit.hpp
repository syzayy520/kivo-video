// encoded_access_unit.hpp — P6A passthrough: EncodedAccessUnit (§8.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
struct EncodedAccessUnit {
    ContractMetadata metadata;
    int32_t codec_id{0};
    int32_t profile{0};
    uint64_t source_packet_id{0};
    int64_t media_start{0};
    int64_t encoded_duration{0};
    int32_t boundary{0};
    int64_t time_base_num{0};
    int64_t time_base_den{0};
    bool sync_frame{false};
    bool discontinuity{false};
    uint64_t payload_handle_id{0};
    uint64_t validation_evidence_id{0};
    bool operator==(const EncodedAccessUnit&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough

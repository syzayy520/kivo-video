// audio_codec_private_data_summary.hpp — P6A format: codec private data summary contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioCodecPrivateDataSummary {
    kivo::video::audio_plane::ContractMetadata metadata;
    int32_t codec_family{0};
    int32_t size_bytes{0};
    int32_t redaction_level{0};
    uint64_t salted_hash{0};
    bool presence{false};
    uint32_t version{0};
};
}  // namespace kivo::video::audio_plane::format

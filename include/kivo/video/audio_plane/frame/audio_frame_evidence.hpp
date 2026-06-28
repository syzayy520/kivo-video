// audio_frame_evidence.hpp — P6A frame: AudioFrameEvidence (§9.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::frame {
struct AudioFrameEvidence {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    uint64_t frame_id{0};
    int32_t kind{0};
    int64_t timestamp_ms{0};
    bool operator==(const AudioFrameEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::frame

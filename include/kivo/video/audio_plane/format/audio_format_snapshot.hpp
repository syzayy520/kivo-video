// audio_format_snapshot.hpp — P6A format: audio format snapshot contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/format/audio_format_descriptor.hpp"
namespace kivo::video::audio_plane::format {
struct AudioFormatSnapshot {
    kivo::video::audio_plane::ContractMetadata metadata;
    AudioFormatDescriptor descriptor;
    uint64_t format_generation{0};
    uint64_t evidence_id{0};
};
}  // namespace kivo::video::audio_plane::format

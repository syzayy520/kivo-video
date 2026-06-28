// audio_data_handle_owner.hpp — P6A §8: ownership tracking
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
namespace kivo::video::audio_plane::data {
enum class AudioDataOwner : int32_t { P5 = 0, P6 = 1, DecodeBackend = 2, RenderQueue = 3, Pool = 4, Released = 5 };
struct AudioDataHandleOwner {
    ContractMetadata metadata;
    AudioDataHandleId handle_id;
    AudioDataOwner current_owner{AudioDataOwner::P5};
    AudioDataOwner previous_owner{AudioDataOwner::P5};
    uint64_t transfer_evidence_id{0};
};
}  // namespace kivo::video::audio_plane::data

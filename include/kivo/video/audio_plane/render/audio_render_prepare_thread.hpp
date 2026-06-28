// audio_render_prepare_thread.hpp — P6A §15: producer thread contract
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
struct AudioRenderPrepareThreadContract {
    ContractMetadata metadata;
    bool is_spsc_producer_only{true};
    bool cannot_access_remainder_slot{true};
    bool produces_audio_render_packets{true};
};
}  // namespace kivo::video::audio_plane::render

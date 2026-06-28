// audio_render_packet.hpp — P6A §10: hot-path render packet (noexcept, no forbidden types)
#pragma once
#include <cstdint>
#include <type_traits>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/generation/audio_pipeline_generation.hpp"
#include "kivo/video/audio_plane/format/audio_format_hash.hpp"
namespace kivo::video::audio_plane::render {
struct AudioRenderPacket {
    ContractMetadata metadata;
    uint64_t pool_slot_handle_id{0};
    int32_t sample_count{0};
    int32_t channel_count{0};
    int64_t pts{0};
    int64_t duration{0};
    generation::AudioPipelineGeneration pipeline_generation;
    format::AudioFormatHash format_hash;
    int32_t remaining_frame_cursor{0};
    uint64_t claim_state_snapshot_id{0};
    bool is_silence{false};
};
static_assert(std::is_nothrow_move_constructible_v<AudioRenderPacket>, "AudioRenderPacket must be nothrow move constructible");
static_assert(std::is_nothrow_destructible_v<AudioRenderPacket>, "AudioRenderPacket must be nothrow destructible");
}  // namespace kivo::video::audio_plane::render

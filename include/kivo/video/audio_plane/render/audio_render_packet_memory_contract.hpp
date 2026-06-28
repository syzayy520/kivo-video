// audio_render_packet_memory_contract.hpp — P6A §10: forbidden/allowed fields
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
struct AudioRenderPacketMemoryContract {
    ContractMetadata metadata;
    bool no_vector{true};
    bool no_string{true};
    bool no_shared_ptr{true};
    bool no_function{true};
    bool no_owning_heap_block{true};
    bool no_backend_com_pointer{true};
    bool no_ffmpeg_pointer{true};
    bool no_qt_type{true};
    bool no_borrowed_span{true};
    bool allow_pool_slot_handle{true};
    bool allow_lightweight_span_descriptor{true};
    bool allow_small_fixed_metadata{true};
    bool allow_generation_ids{true};
    bool allow_format_hash{true};
    bool allow_remaining_frame_cursor{true};
    bool allow_claim_state_snapshot_id{true};
    bool partial_consumption_via_remainder_slot{true};
    bool no_push_back_into_spsc{true};
};
}  // namespace kivo::video::audio_plane::render

// audio_packet_import_view.hpp — P6A §7: P6-side import view
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"
namespace kivo::video::audio_plane::packet {
struct AudioPacketImportView {
    ContractMetadata metadata;
    AudioStreamId stream_id;
    AudioTrackId track_id;
    AudioPacketId packet_id;
    AudioPacketSequence packet_sequence;
    int64_t pts{0};
    int64_t dts{0};
    int64_t duration{0};
    int64_t time_base_num{0};
    int64_t time_base_den{0};
    uint64_t decode_order_index{0};
    int64_t media_time_start{0};
    int64_t media_time_end{0};
    bool is_key_packet{false};
    bool is_decoder_safe_boundary{false};
    bool is_discontinuity{false};
    bool is_eos{false};
    bool is_flush_marker{false};
    int32_t codec_id{0};
    int32_t codec_profile{0};
    uint64_t codec_private_data_ref{0};
    int32_t side_data_summary{0};
    bool encrypted_or_protected{false};
    int32_t protected_hint{0};
    int32_t data_source{0};
    int32_t ownership_mode{0};
    uint64_t engine_epoch{0};
    uint64_t pipeline_generation{0};
    uint64_t source_evidence_id{0};
};
}  // namespace kivo::video::audio_plane::packet

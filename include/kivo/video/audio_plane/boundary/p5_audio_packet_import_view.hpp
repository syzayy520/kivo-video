// p5_audio_packet_import_view.hpp — P6A boundary: P5 audio packet import view (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P5AudioPacketImportView
// Read-only view into an audio packet crossing the P5 (native decode) to P6
// (audio media plane) boundary. All fields are value-semantic; no ownership of
// packet payload is transferred through this view. IDs are strong uint64_t
// tokens; timestamps are media-time-relative and int64_t for signed deltas.
struct P5AudioPacketImportView {
    kivo::video::audio_plane::ContractMetadata metadata{};

    // Identity / genealogy
    uint64_t stream_id{0};
    uint64_t track_id{0};
    uint64_t packet_id{0};
    uint64_t packet_sequence{0};

    // Timing (media-time-relative, int64_t for signed deltas)
    int64_t pts{0};
    int64_t dts{0};
    int64_t duration{0};
    int64_t time_base{0};

    // Ordering / decode chain
    uint64_t decode_order_index{0};
    int64_t media_time_range{0};

    // Boundary semantics (flags)
    bool is_key_packet{false};
    bool is_decoder_safe_boundary{false};
    bool is_discontinuity{false};
    bool is_eos{false};
    bool is_flush_marker{false};

    // Codec descriptors (enum-as-int placeholders; no platform types)
    int32_t codec_id{0};
    int32_t codec_profile{0};
    uint64_t codec_private_data_ref{0};
    uint64_t side_data_summary{0};

    // Protection / provenance
    bool encrypted_or_protected{false};
    int32_t protected_hint{0};
    int32_t data_source{0};
    int32_t ownership_mode{0};

    // Engine genealogy
    uint64_t engine_epoch{0};
    uint64_t pipeline_generation{0};
    uint64_t source_evidence_id{0};

    bool operator==(const P5AudioPacketImportView&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary

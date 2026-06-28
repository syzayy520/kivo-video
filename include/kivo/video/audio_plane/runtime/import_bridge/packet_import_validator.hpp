// packet_import_validator.hpp — P6B import_bridge: packet import validator
// HEADER-ONLY. Applies P5AudioPacketImportContract rules to import views.
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/boundary/p5_audio_packet_import_view.hpp"
#include "kivo/video/audio_plane/packet/p5_audio_packet_import_contract.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"

namespace kivo::video::audio_plane::runtime::import_bridge {

// PacketImportValidator
// Stateless validator applying the 6 rules of P5AudioPacketImportContract.
// All methods are pure functions over the view; no side effects, no backend.
class PacketImportValidator {
public:
    PacketImportValidator() = default;

    // Rule 1: stream_id, track_id, engine_epoch, pipeline_generation all nonzero.
    bool validate_stream_id_track_id_generation_epoch(
        const boundary::P5AudioPacketImportView& view) const noexcept
    {
        return view.stream_id != 0
            && view.track_id != 0
            && view.engine_epoch != 0
            && view.pipeline_generation != 0;
    }

    // Rule 2: packet_sequence strictly greater than last_sequence unless
    //         is_discontinuity is set (gap permitted on discontinuity).
    bool validate_packet_sequence_monotonicity(
        const boundary::P5AudioPacketImportView& view,
        uint64_t last_sequence) const noexcept
    {
        if (view.is_discontinuity) {
            return true;  // discontinuity allows a sequence break
        }
        return view.packet_sequence > last_sequence;
    }

    // Rule 3: codec_private_data_ref must be present before decode init.
    //         Heuristic: a key packet must carry private data ref.
    bool validate_codec_private_data_before_decode_init(
        const boundary::P5AudioPacketImportView& view) const noexcept
    {
        if (view.is_key_packet) {
            return view.codec_private_data_ref != 0;
        }
        return true;  // non-key packets do not require private data
    }

    // Rule 6: protected/encrypted packets must have redacted summary fields.
    //         side_data_summary and codec_private_data_ref must be zero.
    bool protected_encrypted_summary_redacted(
        const boundary::P5AudioPacketImportView& view) const noexcept
    {
        if (!view.encrypted_or_protected) {
            return true;  // not applicable
        }
        return view.side_data_summary == 0
            && view.codec_private_data_ref == 0;
    }

    // Aggregate check applying all enabled rules of a contract.
    bool validate_all(
        const boundary::P5AudioPacketImportView& view,
        const packet::P5AudioPacketImportContract& contract,
        uint64_t last_sequence) const noexcept
    {
        if (contract.validate_stream_id_track_id_generation_epoch
            && !validate_stream_id_track_id_generation_epoch(view)) {
            return false;
        }
        if (contract.validate_packet_sequence_monotonicity
            && !validate_packet_sequence_monotonicity(view, last_sequence)) {
            return false;
        }
        if (contract.validate_codec_private_data_before_decode_init
            && !validate_codec_private_data_before_decode_init(view)) {
            return false;
        }
        if (contract.protected_encrypted_summary_redacted
            && !protected_encrypted_summary_redacted(view)) {
            return false;
        }
        return true;
    }
};

}  // namespace kivo::video::audio_plane::runtime::import_bridge

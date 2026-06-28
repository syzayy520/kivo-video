// p5_audio_packet_import_contract.hpp — P6A §7: P5→P6 import contract
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::packet {
struct P5AudioPacketImportContract {
    ContractMetadata metadata;
    bool validate_stream_id_track_id_generation_epoch{true};
    bool validate_packet_sequence_monotonicity{true};
    bool validate_codec_private_data_before_decode_init{true};
    bool never_store_p5_raw_pointer{true};
    bool never_expose_packet_bytes_to_p8{true};
    bool protected_encrypted_summary_redacted{true};
};
}  // namespace kivo::video::audio_plane::packet

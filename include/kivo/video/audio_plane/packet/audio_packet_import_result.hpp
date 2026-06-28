// audio_packet_import_result.hpp — P6A §7: 12-value import result
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::packet {
enum class AudioPacketImportResult : int32_t {
    AcceptedMoved = 0, AcceptedCopied = 1, AcceptedPoolSlot = 2,
    DeferredBackpressure = 3,
    RejectedInvalidGeneration = 4, RejectedInvalidOwnership = 5,
    RejectedMissingPrivateData = 6, RejectedPacketOrderViolation = 7,
    RejectedProtectedPolicy = 8, RejectedOversize = 9,
    RejectedUnsupportedCodec = 10, SupersededByFlushOrSeek = 11,
};
inline bool is_accepted(AudioPacketImportResult r) { return r == AudioPacketImportResult::AcceptedMoved || r == AudioPacketImportResult::AcceptedCopied || r == AudioPacketImportResult::AcceptedPoolSlot; }
inline bool is_rejected(AudioPacketImportResult r) { return static_cast<int32_t>(r) >= 4 && static_cast<int32_t>(r) <= 10; }
inline bool is_deferred(AudioPacketImportResult r) { return r == AudioPacketImportResult::DeferredBackpressure; }
inline bool is_superseded(AudioPacketImportResult r) { return r == AudioPacketImportResult::SupersededByFlushOrSeek; }
}  // namespace kivo::video::audio_plane::packet

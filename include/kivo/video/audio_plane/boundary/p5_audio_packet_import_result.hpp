// p5_audio_packet_import_result.hpp — P6A boundary: P5 packet import result enum (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P5AudioPacketImportResult
// Outcome of attempting to import a single P5 audio packet across the boundary
// into the P6 audio media plane. Values partition into Accepted, Deferred, and
// Rejected categories. No platform error codes; pure semantic enum.
enum class P5AudioPacketImportResult : int32_t {
    AcceptedMoved                 = 0,
    AcceptedCopied                = 1,
    AcceptedPoolSlot              = 2,
    DeferredBackpressure          = 3,
    RejectedInvalidGeneration     = 4,
    RejectedInvalidOwnership      = 5,
    RejectedMissingPrivateData    = 6,
    RejectedPacketOrderViolation  = 7,
    RejectedProtectedPolicy       = 8,
    RejectedOversize              = 9,
    RejectedUnsupportedCodec      = 10,
    SupersededByFlushOrSeek       = 11,
};

}  // namespace kivo::video::audio_plane::boundary

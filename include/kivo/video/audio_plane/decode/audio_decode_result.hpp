// audio_decode_result.hpp — P6A decode: AudioDecodeStatus + AudioDecodeResult (§7.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
enum class AudioDecodeStatus {
    Ok = 0,
    NeedMoreData = 1,
    TimedOut = 2,
    FailedRecoverable = 3,
    FailedFatal = 4,
    Superseded = 5,
    FormatChanged = 6,
};
struct AudioDecodeResult {
    ContractMetadata metadata;
    AudioDecodeStatus status{AudioDecodeStatus::Ok};
    int32_t frames_decoded{0};
    uint64_t frame_handle_id{0};
    uint64_t evidence_id{0};
    bool operator==(const AudioDecodeResult&) const = default;
};
}  // namespace kivo::video::audio_plane::decode

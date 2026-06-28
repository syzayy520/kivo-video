// p6_recovery_seek_request.hpp — P6A boundary: P6 recovery seek request (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P6RecoverySeekRequest
// Internal recovery seek issued by the P6 audio media plane. Distinct from a
// user-initiated seek: excluded_from_user_history defaults to true so the
// recovery seek does not pollute the user's playback history. correlation_id
// ties the recovery to the fault that triggered it.
struct P6RecoverySeekRequest {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t request_id{0};
    int64_t target_media_time_ms{0};
    int32_t seek_precision{0};
    bool excluded_from_user_history{true};
    uint64_t correlation_id{0};

    bool operator==(const P6RecoverySeekRequest&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary

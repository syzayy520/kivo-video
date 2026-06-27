// recovery_seek_request.hpp — GATE-009: P5A recovery seek request (9 fields, strong types)
#pragma once
#include <cstdint>
#include <string>
#include "../stamp/core_video_stamp.hpp"

namespace kivo::video_plane::recovery {

using kivo::video_plane::stamp::CoreVideoStamp;

struct RecoverySeekRequest {
    uint64_t timeline_revision{0};          // P4 timeline revision at request time
    uint64_t target_media_time{0};          // target media timestamp (NOT position)
    std::string target_keyframe_policy;     // "NearestBefore"/"NearestAfter"/"ExactOnly"
    std::string active_video_track;         // video track id
    std::string active_audio_track_boundary;// audio track boundary (P5 does NOT own audio)
    std::string active_subtitle_track_boundary;// subtitle track boundary (P5 does NOT own subtitle)
    std::string recovery_reason;            // machine-readable reason code
    bool user_visible{false};               // ALWAYS false — recovery is invisible
    std::string correlation_id;

    // Contract:
    // - RecoverySeekRequest does NOT enter user seek history
    // - P4 is still the playback position authority
    // - P4 may reject RecoverySeekRequest
    // - On rejection: P5 must StandbyUntilNewInput or FailToUpperLayer
    bool operator==(RecoverySeekRequest const&) const = default;
};

}  // namespace kivo::video_plane::recovery

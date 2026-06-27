// recovery_bookmark.hpp -- P5E E8: RecoveryBookmark and RecoverySeekRequest runtime
#pragma once
#include <cstdint>
#include <string>
#include "../recovery/recovery_seek_request.hpp"

namespace kivo::video_plane::timing {

using kivo::video_plane::recovery::RecoverySeekRequest;

struct RecoveryBookmark {
    uint64_t bookmark_id{0};
    int64_t media_time_ms{0};           // media time at bookmark
    uint64_t pipeline_revision{0};      // pipeline revision at bookmark
    int64_t live_origin_snapshot{0};    // live edge origin snapshot
    std::string keyframe_track_id;      // video track for keyframe seek
    bool is_valid{false};

    bool operator==(RecoveryBookmark const&) const = default;
};

enum class RecoverySeekOutcome {
    Accepted,
    RejectedByP4,
    StandbyUntilNewInput,
    FailToUpperLayer
};

class FakeRecoverySeekEngine {
public:
    RecoveryBookmark create_bookmark(int64_t media_time, uint64_t revision) {
        RecoveryBookmark bm;
        bm.bookmark_id = ++next_bookmark_id_;
        bm.media_time_ms = media_time;
        bm.pipeline_revision = revision;
        bm.live_origin_snapshot = media_time;
        bm.is_valid = true;
        last_bookmark_ = bm;
        return bm;
    }

    RecoverySeekOutcome submit_seek(RecoverySeekRequest const& req) {
        last_request_ = req;
        // P4 is authority; simulate P4 accepting if bookmark is recent
        if (last_bookmark_.is_valid && req.timeline_revision == last_bookmark_.pipeline_revision) {
            return RecoverySeekOutcome::Accepted;
        }
        // P4 rejected -- must go to Standby or Fail
        return RecoverySeekOutcome::RejectedByP4;
    }

    RecoverySeekOutcome handle_rejection() {
        return RecoverySeekOutcome::StandbyUntilNewInput;
    }

    RecoveryBookmark const& last_bookmark() const { return last_bookmark_; }
    RecoverySeekRequest const& last_request() const { return last_request_; }

private:
    uint64_t next_bookmark_id_{0};
    RecoveryBookmark last_bookmark_;
    RecoverySeekRequest last_request_;
};

}  // namespace kivo::video_plane::timing

// live_timestamp_runtime.hpp -- P5E E9: LiveTimestampOriginResolver runtime (7-level priority)
// Priority: 1.P4-approved > 2.P4TimelineOriginHint > 3.RecoveryBookmark >
//           4.segment absolute time > 5.PCR/PTS > 6.ConservativeLiveRebase > 7.TimestampInvalid
#pragma once
#include <cstdint>
#include <string>
#include <optional>

namespace kivo::video_plane::timing {

enum class LiveTimestampSource {
    P4Approved,
    P4TimelineOriginHint,
    RecoveryBookmark,
    SegmentAbsoluteTime,
    PcrPts,
    ConservativeLiveRebase,
    TimestampInvalid
};

struct LiveTimestampResolution {
    LiveTimestampSource source{LiveTimestampSource::TimestampInvalid};
    int64_t resolved_origin_ms{0};
    bool is_valid{false};
    std::string resolution_reason;
};

class FakeLiveTimestampResolver {
public:
    void set_p4_approved(int64_t origin) {
        p4_approved_ = origin;
        has_p4_approved_ = true;
    }
    void set_p4_timeline_hint(int64_t origin) {
        p4_hint_ = origin;
        has_p4_hint_ = true;
    }
    void set_recovery_bookmark(int64_t origin) {
        recovery_bookmark_ = origin;
        has_recovery_bookmark_ = true;
    }
    void set_segment_absolute(int64_t origin) {
        segment_absolute_ = origin;
        has_segment_absolute_ = true;
    }
    void set_pcr_pts(int64_t origin) {
        pcr_pts_ = origin;
        has_pcr_pts_ = true;
    }
    void set_conservative_rebase(int64_t origin) {
        conservative_rebase_ = origin;
        has_conservative_rebase_ = true;
    }

    // Resolve with 7-level priority
    LiveTimestampResolution resolve() {
        LiveTimestampResolution r;
        if (has_p4_approved_) {
            r.source = LiveTimestampSource::P4Approved;
            r.resolved_origin_ms = p4_approved_;
            r.is_valid = true;
            r.resolution_reason = "p4_approved";
            return r;
        }
        if (has_p4_hint_) {
            r.source = LiveTimestampSource::P4TimelineOriginHint;
            r.resolved_origin_ms = p4_hint_;
            r.is_valid = true;
            r.resolution_reason = "p4_timeline_origin_hint";
            return r;
        }
        if (has_recovery_bookmark_) {
            r.source = LiveTimestampSource::RecoveryBookmark;
            r.resolved_origin_ms = recovery_bookmark_;
            r.is_valid = true;
            r.resolution_reason = "recovery_bookmark";
            return r;
        }
        if (has_segment_absolute_) {
            r.source = LiveTimestampSource::SegmentAbsoluteTime;
            r.resolved_origin_ms = segment_absolute_;
            r.is_valid = true;
            r.resolution_reason = "segment_absolute_time";
            return r;
        }
        if (has_pcr_pts_) {
            r.source = LiveTimestampSource::PcrPts;
            r.resolved_origin_ms = pcr_pts_;
            r.is_valid = true;
            r.resolution_reason = "pcr_pts";
            return r;
        }
        if (has_conservative_rebase_) {
            r.source = LiveTimestampSource::ConservativeLiveRebase;
            r.resolved_origin_ms = conservative_rebase_;
            r.is_valid = true;
            r.resolution_reason = "conservative_live_rebase";
            return r;
        }
        r.source = LiveTimestampSource::TimestampInvalid;
        r.is_valid = false;
        r.resolution_reason = "no_valid_source";
        return r;
    }

private:
    int64_t p4_approved_{0}, p4_hint_{0}, recovery_bookmark_{0};
    int64_t segment_absolute_{0}, pcr_pts_{0}, conservative_rebase_{0};
    bool has_p4_approved_{false}, has_p4_hint_{false}, has_recovery_bookmark_{false};
    bool has_segment_absolute_{false}, has_pcr_pts_{false}, has_conservative_rebase_{false};
};

}  // namespace kivo::video_plane::timing

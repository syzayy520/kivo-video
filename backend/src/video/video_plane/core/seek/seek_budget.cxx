#include "video/video_plane/core/seek/seek_budget.hpp"

namespace kivo::video::core::seek {

SeekAssessment assess_seek(const SeekFacts& facts, std::int64_t budget_us) {
    SeekAssessment out;

    // 没发生 seek 时**不能**判"通过"。这一条是本卡最容易犯的错：
    // 一轮没有 seek 的证据，所有字段都是 -1，看起来"没有超预算"。
    if (facts.requests <= 0) {
        out.reason =
            "no seek happened in this run; the 150 ms budget cannot be judged from a run without "
            "a seek";
        return out;
    }
    if (facts.landing_us < 0) {
        out.reason = "the extractor rejected the seek (landing_us = -1)";
        return out;
    }
    // PREVIOUS_SYNC 的语义就是"落在目标之前"。落在后面说明参数或实现有问题 ——
    // 那种情况下即使时间达标，后续帧也要先丢掉一批，数字不能算数。
    if (facts.landing_us > facts.target_us) {
        out.reason = "landing " + std::to_string(facts.landing_us) + " us is after target " +
                     std::to_string(facts.target_us) +
                     " us; PREVIOUS_SYNC must land at or before the target";
        return out;
    }
    if (facts.first_frame_us < 0) {
        out.reason = "no frame was presented after the seek (first_frame_us = -1)";
        return out;
    }
    if (facts.prepare_us < 0) {
        out.reason = "seek preparation time was not measured (prepare_us = -1)";
        return out;
    }

    out.total_us = facts.prepare_us + facts.first_frame_us;

    // P16-P1B8：首帧 pts 落在 target **之后**，说明"落点 ≤ target"这条语义没兑现。
    //
    // 这不是"解码慢"，是实际起点跑到了后面：解封装器按 PREVIOUS_SYNC 落 0，
    // 但 flush 后必须等下一个关键帧才能出帧，于是首帧 pts 成了 9 s 而 first_frame_us
    // 被算成 9 秒。把两件事混在一起，读证据的人会被引向错误的排查方向。
    //
    // 历史 42 轮里这条分支一次都没触发（全部 pts ≤ target），所以不重算任何历史 pass。
    if (facts.video_first_pts_us >= 0 && facts.target_us >= 0 &&
        facts.video_first_pts_us > facts.target_us) {
        out.reason = "first frame pts " + std::to_string(facts.video_first_pts_us) + " us is " +
                     std::to_string(facts.video_first_pts_us - facts.target_us) +
                     " us after target " + std::to_string(facts.target_us) +
                     " us (extractor landed at " + std::to_string(facts.landing_us) +
                     " us) — the decoder started at a later keyframe, so this is a landing / "
                     "keyframe-spacing problem, not decode latency (total " +
                     std::to_string(out.total_us) + " us)";
        return out;
    }

    if (out.total_us <= budget_us) {
        out.pass = true;
        return out;
    }
    out.reason = "total " + std::to_string(out.total_us) + " us exceeds the " +
                 std::to_string(budget_us) + " us budget (prepare " +
                 std::to_string(facts.prepare_us) + " + first frame " +
                 std::to_string(facts.first_frame_us) + ")";
    return out;
}

ColdStartAssessment assess_cold_start(std::int64_t first_frame_us, std::int64_t budget_us) {
    ColdStartAssessment out;
    out.budget_us = budget_us;

    if (first_frame_us < 0) {
        // 没出帧不是"通过"，也不是"超时" —— 是"没测到"。三态，别压成两态。
        out.reason = "no frame was delivered before the session stopped; cold start cannot be judged";
        return out;
    }
    out.applicable = true;
    out.first_frame_us = first_frame_us;

    if (first_frame_us > budget_us) {
        out.reason = "cold start took " + std::to_string(first_frame_us) + " us, over the " +
                     std::to_string(budget_us) +
                     " us budget (G8a; this is the first start after open, not a seek)";
        return out;
    }
    out.pass = true;
    return out;
}

AvSeekAssessment assess_av_seek(const SeekFacts& facts, std::int64_t budget_us,
                               std::int64_t max_gap_us) {
    AvSeekAssessment out;
    out.video = assess_seek(facts, budget_us);

    if (!facts.audio_present) {
        out.applicable = false;
        out.reason =
            "source has no audio track (or audio was not attached); the A/V criterion does not "
            "apply to this run — this is not a pass";
        return out;
    }
    out.applicable = true;

    if (!out.video.pass) {
        out.reason = "video side failed: " + out.video.reason;
        return out;
    }
    if (facts.audio_landing_us < 0) {
        out.reason = "audio side did not complete the seek (landing_us = -1)";
        return out;
    }

    out.audio_gap_us = facts.audio_landing_us - facts.landing_us;
    const std::int64_t magnitude =
        out.audio_gap_us < 0 ? -out.audio_gap_us : out.audio_gap_us;
    if (magnitude > max_gap_us) {
        out.reason = "audio landed at " + std::to_string(facts.audio_landing_us) +
                     " us while video landed at " + std::to_string(facts.landing_us) +
                     " us (gap " + std::to_string(out.audio_gap_us) + " us, limit " +
                     std::to_string(max_gap_us) + " us) — the two sides are out of sync";
        return out;
    }
    out.pass = true;
    return out;
}

}  // namespace kivo::video::core::seek

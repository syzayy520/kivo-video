#include "video/video_plane/backends/mediacodec/session/decode_pace_report.hpp"

namespace kivo::video::mediacodec {

DecodePaceStats decode_pace_stats_from(const core::pace::PaceSummary& summary,
                                       std::int64_t snapshot_at_frames) {
    DecodePaceStats out;
    out.enabled = true;
    out.clock_source = summary.clock_source != nullptr ? summary.clock_source : "unknown";
    out.frame_duration_ns = summary.frame_duration_ns;
    out.derived_fps = summary.derived_fps;

    out.frames_decided = summary.frames_decided;
    out.presented = summary.presented;
    out.dropped = summary.dropped;
    out.drop_ratio = summary.drop_ratio;
    out.catch_up_events = summary.catch_up_events;
    out.catch_up_frames = summary.catch_up_frames;

    out.extrapolated_frames = summary.extrapolated_frames;
    out.free_run_frames = summary.free_run_frames;
    out.jump_rebases = summary.jump_rebases;
    out.monotonic_rejects = summary.monotonic_rejects;
    out.stale_readings = summary.stale_readings;
    out.waited = summary.waited;
    out.wait_total_ns = summary.wait_total_ns;

    out.latency_samples = summary.latency_samples;
    out.lateness_p50_ns = summary.lateness.percentile_ns(0.50);
    out.lateness_p95_ns = summary.lateness.percentile_ns(0.95);
    out.lateness_p99_ns = summary.lateness.percentile_ns(0.99);
    out.lateness_min_ns = summary.lateness.min_ns();
    out.lateness_max_ns = summary.lateness.max_ns();

    out.interval_p50_ns = summary.interval.percentile_ns(0.50);
    out.interval_p95_ns = summary.interval.percentile_ns(0.95);
    out.interval_p99_ns = summary.interval.percentile_ns(0.99);
    out.interval_max_ns = summary.interval.max_ns();

    out.over_budget_frames = summary.over_budget_frames;

    // P14-L：可测性原样转发。这一层**不重新判断**任何条件 ——
    // 判断属于 `core::pace`（那里知道 `lateness_` 有没有样本、
    // `frames_decided_` 是多少）。这里再判一次，就是第二份条件。
    out.present_timestamp_measured = summary.present_timestamp_measured;
    out.over_budget_measured = summary.over_budget_measured;
    out.catch_up_measured = summary.catch_up_measured;
    out.drop_ratio_measured = summary.drop_ratio_measured;

    out.scheduled_samples = summary.scheduled_samples;
    if (summary.schedule_lead.count() > 0) {
        out.schedule_lead_p01_ns = summary.schedule_lead.percentile_ns(0.01);
        out.schedule_lead_p10_ns = summary.schedule_lead.percentile_ns(0.10);
        out.schedule_lead_p50_ns = summary.schedule_lead.percentile_ns(0.50);
        out.schedule_lead_p90_ns = summary.schedule_lead.percentile_ns(0.90);
        out.schedule_lead_p99_ns = summary.schedule_lead.percentile_ns(0.99);
        out.schedule_lead_min_ns = summary.schedule_lead.min_ns();
        out.schedule_lead_max_ns = summary.schedule_lead.max_ns();
        // 稳健跨度：排除开播头几帧的锚点瞬态，见 decode_pace_stats.hpp 的注释。
        out.schedule_lead_spread_ns =
            out.schedule_lead_p90_ns - out.schedule_lead_p10_ns;
    }

    out.snapshot_at_frames = snapshot_at_frames;

    // P12-C8：逐帧时钟分档。
    out.clock_primary_frames = summary.clock_primary_frames;
    out.clock_fallback_frames = summary.clock_fallback_frames;
    out.clock_system_frames = summary.clock_system_frames;
    out.first_present_mono_ns = summary.first_present_mono_ns;
    out.last_present_mono_ns = summary.last_present_mono_ns;

    return out;
}

}  // namespace kivo::video::mediacodec

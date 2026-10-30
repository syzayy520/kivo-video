#include "video/video_plane/backends/avsync/report/av_sync_report.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_json_string_escaper.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"

#include <cstdio>
#include <sstream>

namespace kivo::video::avsync {
namespace {

using core::pace::SignedHistogram;

/// 版本历史：
///   /1（C8，首版）：把 C1–C7 的事实收进一份文件。
///
/// 与 `kivo.p12c.decode/5` 的关系：那个只讲视频侧，这个讲"音视频一起跑的时候"。
/// 两者不是替代关系 —— `decode/5` 是纯视频基线，`sync/1` 才回答 G7。
constexpr const char* kSchema = "kivo.p12c.sync/1";

[[nodiscard]] std::string fixed6(double value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    return std::string(buffer);
}

/// 变量名直出用，避免到处写 `<< escape_json_string(...)` 把行拉得很长。
[[nodiscard]] std::string quoted(const std::string& text) {
    return "\"" + mediacodec::escape_json_string(text) + "\"";
}

/// 渲染一段完整分布。
///
/// 这是 C8 "可复算" 的物证：只给 p99，读者只能选择相信；给分布，
/// p99 就退化成一道可以复核的算术题。三个常量（桶宽、下界、桶数）一并导出，
/// 复算方不需要去翻源码里的魔数。
///
/// 非空桶写成 `{"<下标>": 计数}`，**没出现的下标一律为 0** —— 1024 个桶
/// 全写出来是 2 万字符的噪声，而实际只会用到几十个。
void render_distribution(std::ostringstream& out, const SignedHistogram& histogram,
                         const char* indent) {
    const std::string pad(indent);
    out << pad << "\"count\": " << histogram.count() << ",\n";
    out << pad << "\"below\": " << histogram.below_count() << ",\n";
    out << pad << "\"above\": " << histogram.above_count() << ",\n";
    out << pad << "\"min_ns\": " << histogram.min_ns() << ",\n";
    out << pad << "\"max_ns\": " << histogram.max_ns() << ",\n";
    out << pad << "\"bucket_width_ns\": " << SignedHistogram::kBucketWidthNs << ",\n";
    out << pad << "\"low_ns\": " << SignedHistogram::kLowNs << ",\n";
    out << pad << "\"bucket_count\": " << SignedHistogram::kBucketCount << ",\n";
    out << pad << "\"buckets\": {";
    bool first = true;
    const auto& bins = histogram.bins();
    for (std::int32_t index = 0; index < SignedHistogram::kBucketCount; ++index) {
        const std::int64_t count = bins[static_cast<std::size_t>(index)];
        if (count == 0) {
            continue;
        }
        out << (first ? "" : ",") << "\"" << index << "\":" << count;
        first = false;
    }
    out << "}\n";
}

/// 分位数 + 分布的组合对象。
void render_quantiles(std::ostringstream& out, const SignedHistogram& histogram,
                      const char* indent) {
    const std::string pad(indent);
    out << pad << "\"samples\": " << histogram.count() << ",\n";
    out << pad << "\"p50\": " << histogram.percentile_ns(0.50) << ",\n";
    out << pad << "\"p95\": " << histogram.percentile_ns(0.95) << ",\n";
    out << pad << "\"p99\": " << histogram.percentile_ns(0.99) << ",\n";
    out << pad << "\"max\": " << histogram.max_ns() << ",\n";
    out << pad << "\"histogram\": {\n";
    render_distribution(out, histogram, (pad + "  ").c_str());
    out << pad << "}\n";
}

}  // namespace

std::string render_av_sync_json(const AvSyncStats& stats) {
    const mediacodec::DecodeStats& video = stats.video;
    const AudioPlaybackStats& audio = stats.audio;
    const core::pace::PaceSummary& pace = stats.pace;

    // 时钟来源只认一件事：**实际驱动呈现的那一个**。
    // 有音频且音频时钟真的发过有效值 → 报音频档；否则老实写 `system`。
    // 用"配置上打算用哪个"来填这一格，等于把降级从证据里抹掉。
    std::string clock_source = "system";
    if (stats.has_audio && audio.clock.queries > 0) {
        clock_source = core::audio::audio_clock_source_name(audio.clock.dominant_source());
    }

    std::string error = video.error;
    if (error.empty()) {
        error = audio.error;
    }
    const bool completed = video.completed && audio.error.empty();

    std::ostringstream out;
    out << "{\n";
    out << "  \"schema\": " << quoted(kSchema) << ",\n";
    out << "  \"generated_at_unix_ms\": " << stats.generated_at_unix_ms << ",\n";

    out << "  \"device\": {\n";
    out << "    \"manufacturer\": " << quoted(stats.device.manufacturer) << ",\n";
    out << "    \"model\": " << quoted(stats.device.model) << ",\n";
    out << "    \"device\": " << quoted(stats.device.device) << ",\n";
    out << "    \"soc_model\": " << quoted(stats.device.soc_model) << ",\n";
    out << "    \"sdk\": " << quoted(stats.device.sdk_int) << ",\n";
    out << "    \"release\": " << quoted(stats.device.release) << ",\n";
    out << "    \"abi\": " << quoted(stats.device.abi) << "\n";
    out << "  },\n";

    out << "  \"codec\": {\n";
    out << "    \"video\": " << quoted(video.codec_name) << ",\n";
    out << "    \"audio\": " << quoted(audio.codec_name) << "\n";
    out << "  },\n";

    out << "  \"source\": {\n";
    out << "    \"label\": " << quoted(stats.source_label) << ",\n";
    out << "    \"bytes\": " << stats.source_bytes << ",\n";
    out << "    \"has_video\": " << (stats.has_video ? "true" : "false") << ",\n";
    out << "    \"has_audio\": " << (stats.has_audio ? "true" : "false") << ",\n";
    out << "    \"duration_us\": " << video.media_duration_us << ",\n";
    out << "    \"video\": {\"mime\": " << quoted(video.mime) << ", \"width\": " << video.width
        << ", \"height\": " << video.height << "},\n";
    out << "    \"audio\": {\"mime\": " << quoted(audio.mime)
        << ", \"sample_rate\": " << audio.declared_sample_rate
        << ", \"channel_count\": " << audio.declared_channel_count << "}\n";
    out << "  },\n";

    // ── 卡片点名的五个标量 ──
    out << "  \"frame_count\": " << video.frames_rendered << ",\n";
    out << "  \"frames_dropped\": " << video.frames_dropped_late << ",\n";
    out << "  \"xrun_count\": " << audio.output.xrun_count << ",\n";
    out << "  \"clock_source\": " << quoted(clock_source) << ",\n";
    out << "  \"timestamp_fallback_count\": "
        << (audio.clock.published_frames_read + audio.clock.published_frames_written +
            audio.clock.published_frozen)
        << ",\n";

    // ── 音画偏差 ──
    //
    // 能不能算，取决于三件事，缺一不可：
    //   ① 片源真有音轨（`system` 时钟下这个量只是"相对理想节拍的偏移"）；
    //   ② 时钟真是音频档；
    //   ③ 呈现路径是 `cpu_paced` —— 帧在 `due_at` **当刻**交出。
    //
    // 第 ③ 条是一次真机测量逼出来的：`kSurfaceAtTime` 不睡，连交 3 帧，
    // 于是 `lateness` 恒等于提交提前量（实测 p50 = −99.875 ms = 3 帧，
    // 174/180 个样本甚至越过了直方图 −128 ms 的下界），跟音画同步没关系。
    //
    // 也就是说：**"提前 3 帧交出去"和"音画不同步"在数值上长得一样，必须靠路径区分。**
    // 上屏时刻的同步误差在 NDK 里拿不到（`21-p12c6b-present-backpressure.md` §二），
    // 要它得等 P12-D 的 present fence。本节宁可写 `measurable: false`，也不编一个数。
    const bool audio_clock = clock_source != "system";
    const bool paced_at_present = video.present_schedule == "cpu_paced";
    const bool drift_measurable =
        stats.has_audio && audio_clock && paced_at_present && pace.lateness.count() > 0;

    out << "  \"av_drift\": {\n";
    if (drift_measurable) {
        out << "    \"measurable\": true,\n";
        out << "    \"measured_at\": \"present\",\n";
        out << "    \"definition\": "
            << quoted("(audio clock elapsed) - (video pts elapsed), sampled at present_at; "
                      "positive means video is behind audio")
            << ",\n";
        render_quantiles(out, pace.lateness, "    ");
    } else {
        out << "    \"measurable\": false,\n";
        out << "    \"samples\": 0,\n";
        out << "    \"unavailable_reason\": ";
        if (!stats.has_audio) {
            out << quoted("source has no audio track; there is no audio timeline to compare "
                          "against");
        } else if (!audio_clock) {
            out << quoted("clock_source is system; the per-frame offset would only measure "
                          "schedule offset, not A/V sync");
        } else if (!paced_at_present) {
            out << quoted("present_schedule is surface_at_time: frames are submitted ~3 frames "
                          "early, so the per-frame offset equals the submit lead (measured "
                          "p50 = -99.875 ms) and is not A/V drift; measuring on-screen sync "
                          "requires present fences (see 21-p12c6b-present-backpressure.md 2)");
        } else {
            out << quoted("no lateness samples were recorded at present time");
        }
        out << "\n";
    }
    out << "  },\n";

    out << "  \"present_jitter\": {\n";
    // 两条路径的采样点不同，必须标明 —— 否则同一组数字会被当成同一件事：
    // `cpu_paced` 记在"交出前的一瞬"，`surface_at_time` 记在"提交给合成器的一瞬"。
    out << "    \"measured_at\": "
        << quoted(video.present_schedule == "cpu_paced" ? "present" : "submit") << ",\n";
    render_quantiles(out, pace.interval, "    ");
    out << "  },\n";

    // ── 播放速率：唯一一个不与节拍器结论循环的检查 ──
    //
    // 分子是媒体时间（首帧→末帧的 pts），分母是墙钟时间（同一对帧的交出时刻）。
    // `av_drift` / `schedule_lead` 都是拿节拍器自己的目标时刻当基准，所以节拍器
    // 整体跑偏时它们可能依然"完美"；这个比值只看墙钟，跑偏就露出来。
    //
    // 刻意**不用**"音频时钟跨度 ÷ 视频 pts 跨度"来算：音频会话总比视频多活一会儿
    // （排空、轮询间隔、收尾），那个差值里混的是收尾耗时，不是分叉。
    // 第一版就是这么写出 35–104 ms 的假"分叉"的，而同轮 `av_drift.p99` 只有 0.125 ms ——
    // 两个数自相矛盾，说明错在窗口。音频停止时刻的原始位置仍如实记录，供人自行核对。
    // 媒体跨度按**交出去的帧数**算，不是按"上了屏的帧数"。
    // 换面空窗期那几十帧没上屏，但 pts 照样走过去了（解码器把它们的缓冲还回来了）。
    // 只数 `frames_rendered` 会把跨度算短，速率就被误判成"放慢了" ——
    // D1 第一版就是这么把 0.92 报出来的，而那 92% 只是空窗期的占比。
    const std::int64_t handed_off = video.frames_rendered + video.frames_dropped_late +
                                    video.frames_discarded_no_surface;
    const std::int64_t video_span_ns =
        (handed_off > 1) ? (handed_off - 1) * pace.frame_duration_ns : 0;
    const std::int64_t wall_span_ns = pace.last_present_mono_ns - pace.first_present_mono_ns;

    // `surface_at_time` 路径的交出时刻比 `due_at` 早一个固定提前量，而且这个提前量
    // 是**从 0 涨到 3 帧**的（首帧建锚时提前量为 0，之后被 cap 卡住）。
    // 于是"首末交出时刻之差"比媒体跨度短了整整一个提前量 —— 直接算速率会读出 1.017，
    // 看起来像"放快了 1.7%"，其实是提前量填进去的。
    //
    // 修正用 `schedule_lead` 的 p50（`cpu_paced` 路径没有样本 → 0 → 不修正）。
    // 这是**减法**不是抹平：修正项单独报出来，读者可以拒绝它自己重算。
    const std::int64_t lead_correction_ns =
        (pace.scheduled_samples > 0 && pace.schedule_lead.percentile_ns(0.50) < 0)
            ? -pace.schedule_lead.percentile_ns(0.50)
            : 0;
    const std::int64_t corrected_span_ns = wall_span_ns + lead_correction_ns;
    const double playback_rate = (corrected_span_ns > 0 && video_span_ns > 0)
                                     ? static_cast<double>(video_span_ns) /
                                           static_cast<double>(corrected_span_ns)
                                     : 0.0;

    out << "  \"timeline\": {\n";
    out << "    \"video_pts_span_ns\": " << video_span_ns << ",\n";
    out << "    \"present_wall_span_ns\": " << wall_span_ns << ",\n";
    out << "    \"lead_correction_ns\": " << lead_correction_ns << ",\n";
    out << "    \"corrected_wall_span_ns\": " << corrected_span_ns << ",\n";
    out << "    \"playback_rate\": " << fixed6(playback_rate) << ",\n";
    // 原始事实照留：有人要按自己的窗口重算时用得上。
    out << "    \"anchor_audio_position_ns\": " << pace.anchor_position_ns << ",\n";
    out << "    \"audio_position_at_video_stop_ns\": "
        << stats.audio_position_at_video_stop_ns << ",\n";
    out << "    \"note\": "
        << quoted("playback_rate = media time / wall time between first and last hand-off, "
                  "with the submit-lead ramp removed. The audio position at video stop "
                  "includes teardown latency (poll interval + drain) and must not be compared "
                  "against the pts span directly")
        << "\n";
    out << "  },\n";

    // ── 锚点：`av_drift` 是相对量，被吸收的常量必须能单独看到 ──
    out << "  \"anchor\": {\n";
    out << "    \"video_pts_ns\": " << pace.anchor_pts_ns << ",\n";
    out << "    \"audio_position_ns\": " << pace.anchor_position_ns << ",\n";
    out << "    \"note\": "
        << quoted("av_drift is measured relative to this anchor; the constant absorbed here "
                  "is audio_position_ns at the first video frame")
        << "\n";
    out << "  },\n";

    // ── 视频侧 ──
    out << "  \"video\": {\n";
    out << "    \"frames_received\": " << video.frames_received << ",\n";
    out << "    \"frames_released\": " << video.frames_released << ",\n";
    out << "    \"frames_rendered\": " << video.frames_rendered << ",\n";
    out << "    \"frames_dropped_late\": " << video.frames_dropped_late << ",\n";
    // D1：换面空窗期丢的帧。与"节拍判它太晚"分开记，否则一次切后台看起来像节拍崩了。
    out << "    \"frames_discarded_no_surface\": " << video.frames_discarded_no_surface << ",\n";
    out << "    \"samples_submitted\": " << video.samples_submitted << ",\n";
    out << "    \"samples_rejected\": " << video.samples_rejected << ",\n";
    out << "    \"wall_us\": " << video.wall_micros << ",\n";
    out << "    \"first_frame_latency_us\": " << video.first_frame_latency_us << ",\n";
    out << "    \"decode_fps\": " << fixed6(video.decode_fps) << ",\n";
    out << "    \"speed_vs_realtime\": " << fixed6(video.speed_vs_realtime) << ",\n";
    out << "    \"pipeline_mode\": " << quoted(video.pipeline_mode) << ",\n";
    out << "    \"present_frames\": " << (video.present_frames ? "true" : "false") << ",\n";
    out << "    \"present_schedule\": " << quoted(video.present_schedule) << ",\n";
    // D1：换面。`rebuilds == 0` 时其余字段保持 -1 —— 不写 0，
    // 0 会被读成"恢复耗时 0ms"。
    out << "    \"surface_rebuild\": {\"rebuilds\": " << video.surface_rebuild.rebinds
        << ", \"gap_ms\": " << video.surface_rebuild.last_gap_ms
        << ", \"resume_ms\": " << video.surface_rebuild.last_resume_ms
        << ", \"frames_lead\": " << video.surface_rebuild.last_frames_lead << "}\n";
    out << "  },\n";

    // ── 节拍侧面 ──
    out << "  \"pace\": {\n";
    // 没有 `enabled` 这个开关可查：节拍是"配了就有"。用 frames_decided 说话，
    // 0 就是没开节拍 —— 不另外编一个可能与事实不符的布尔量。
    out << "    \"enabled\": " << (pace.frames_decided > 0 ? "true" : "false") << ",\n";
    out << "    \"frames_decided\": " << pace.frames_decided << ",\n";
    out << "    \"presented\": " << pace.presented << ",\n";
    out << "    \"dropped\": " << pace.dropped << ",\n";
    out << "    \"over_budget_frames\": " << pace.over_budget_frames << ",\n";
    out << "    \"frame_duration_ns\": " << pace.frame_duration_ns << ",\n";
    out << "    \"derived_fps\": " << fixed6(pace.derived_fps) << ",\n";
    out << "    \"lateness_ns\": {\"samples\": " << pace.latency_samples << ", \"p50\": "
        << pace.lateness.percentile_ns(0.50) << ", \"p99\": " << pace.lateness.percentile_ns(0.99)
        << ", \"max\": " << pace.lateness.max_ns() << "},\n";
    out << "    \"schedule_lead_ns\": {\"samples\": " << pace.scheduled_samples << ", \"p50\": "
        << pace.schedule_lead.percentile_ns(0.50) << ", \"spread\": "
        << (pace.schedule_lead.percentile_ns(0.90) - pace.schedule_lead.percentile_ns(0.10))
        << "},\n";
    out << "    \"clock_health\": {\"extrapolated_frames\": " << pace.extrapolated_frames
        << ", \"free_run_frames\": " << pace.free_run_frames << ", \"jump_rebases\": "
        << pace.jump_rebases << ", \"monotonic_rejects\": " << pace.monotonic_rejects << "},\n";
    out << "    \"clock_mix\": {\"primary_frames\": " << pace.clock_primary_frames
        << ", \"fallback_frames\": " << pace.clock_fallback_frames << ", \"system_frames\": "
        << pace.clock_system_frames << "}\n";
    out << "  },\n";

    // ── 音频侧 ──
    out << "  \"audio\": {\n";
    out << "    \"opened\": " << (audio.opened ? "true" : "false") << ",\n";
    out << "    \"started\": " << (audio.started ? "true" : "false") << ",\n";
    out << "    \"negotiated\": {\"sample_rate\": " << audio.negotiated_sample_rate
        << ", \"channel_count\": " << audio.negotiated_channel_count << ", \"format\": "
        << quoted(audio.negotiated_format) << ", \"sharing\": " << quoted(audio.sharing_mode)
        << ", \"performance\": " << quoted(audio.performance_mode) << ", \"frames_per_burst\": "
        << audio.frames_per_burst << "},\n";
    out << "    \"output_format\": {\"sample_rate\": " << audio.output_format.sample_rate
        << ", \"channel_count\": " << audio.output_format.channel_count << ", \"encoding\": "
        << quoted(std::string(core::audio::pcm_encoding_name(audio.output_format.encoding)))
        << ", \"diverged\": " << (audio.output_format_diverged ? "true" : "false") << "},\n";
    out << "    \"ring_capacity_frames\": " << audio.ring_capacity_frames << ",\n";
    out << "    \"prefill_reached\": " << (audio.prefill_reached ? "true" : "false") << ",\n";
    out << "    \"startup_millis\": " << audio.startup_millis << ",\n";
    out << "    \"clock_start_ns\": " << audio.clock_start_ns << ",\n";
    out << "    \"clock_end_ns\": " << audio.clock_end_ns << ",\n";
    out << "    \"feed\": {\"packets_submitted\": " << audio.feed.packets_submitted
        << ", \"chunks_received\": " << audio.feed.chunks_received << ", \"frames_decoded\": "
        << audio.feed.frames_decoded << ", \"frames_written\": " << audio.feed.frames_written
        << ", \"frames_dropped\": " << audio.feed.frames_dropped << ", \"ring_full_waits\": "
        << audio.feed.ring_full_waits << ", \"reached_eos\": "
        << (audio.feed.reached_eos ? "true" : "false") << "},\n";
    out << "    \"output\": {\"callbacks\": " << audio.output.callbacks
        << ", \"frames_rendered\": " << audio.output.frames_rendered
        << ", \"frames_silenced\": " << audio.output.frames_silenced
        << ", \"silence_head_frames\": " << audio.output.silence_head_frames
        << ", \"silence_tail_frames\": " << audio.output.silence_tail_frames
        << ", \"first_audible_frame\": " << audio.output.first_audible_frame
        << ", \"xrun_count\": " << audio.output.xrun_count << ", \"wall_millis\": "
        << audio.output.wall_millis << "},\n";
    out << "    \"clock\": {\"queries\": " << audio.clock.queries << ", \"published_timestamp\": "
        << audio.clock.published_timestamp << ", \"published_frames_read\": "
        << audio.clock.published_frames_read << ", \"published_frames_written\": "
        << audio.clock.published_frames_written << ", \"published_frozen\": "
        << audio.clock.published_frozen << ", \"timestamp_not_started\": "
        << audio.clock.timestamp_not_started << ", \"timestamp_fail\": "
        << audio.clock.timestamp_fail << ", \"monotonic_rejects\": "
        << audio.clock.monotonic_rejects << ", \"jumps_clamped\": " << audio.clock.jumps_clamped
        << ", \"latency_frames\": " << audio.clock.latency_frames << "}\n";
    out << "  },\n";

    out << "  \"completed\": " << (completed ? "true" : "false") << ",\n";
    out << "  \"error\": " << quoted(error) << "\n";
    out << "}\n";
    return out.str();
}

}  // namespace kivo::video::avsync

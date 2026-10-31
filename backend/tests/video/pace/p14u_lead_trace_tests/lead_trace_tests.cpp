// lead_trace_tests.cpp — P14-U tests: 「钟偏了多少」之外，还要能说出「怎么偏的」
//
// 这一组守六句话：
//   ① 钟与墙 1:1 时领先量恒 0，`verdict` 是 `steady` —— 不是"看起来很小"，是逐位 0。
//   ② 一次阶跃必须被认成 `stepped`，且 `max_step_ns` 就是台阶大小。
//   ③ 匀速跑偏必须被认成 `ramped`，且 `step_frames == 0` —— 两者不许混。
//   ④ **缺参数不许给结论**：没给帧长时是 `unknown`，不是 `ramped`。
//      反过来的话，一个"没量到"会被报成"钟在匀速跑偏"（要修硬件），方向完全错。
//   ⑤ 台阶**正好卡在容差线上**时不算跳（0.9 帧），也不许算成跑偏 —— 边界是边界。
//   ⑥ 接线必须真接上：`PaceStats::note_presented` 要把两个记录器喂到，
//      而且 `surface_at_time` 档（不带钟读数）**一个样本都不许喂** ——
//      喂进去 480 个 0 会得到 `max_step = 0 / verdict = steady`，一个漂亮的自信心。
//
// 纯桌面计算：假时间轴，不依赖真实时间、不需要设备。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_lead_trace.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"
#include "video/video_plane/core/pace/pace_wall_offset.hpp"

namespace pace = kivo::video::core::pace;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                       \
    do {                                                                  \
        ++g_checked;                                                      \
        if (!(expr)) {                                                    \
            ++g_failed;                                                   \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr); \
        }                                                                 \
    } while (false)

#define CHECK_EQ(actual, expected)                                              \
    do {                                                                        \
        ++g_checked;                                                            \
        const long long a = static_cast<long long>(actual);                      \
        const long long e = static_cast<long long>(expected);                    \
        if (a != e) {                                                           \
            ++g_failed;                                                         \
            std::printf("  FAIL %s:%d  %lld vs %lld\n", __FILE__, __LINE__, a, e); \
        }                                                                       \
    } while (false)

constexpr std::int64_t kFrameNs = 16'666'667;  // 60 fps

/// 一条"钟与墙同步走"的时间轴：每帧墙走 T，钟也走 T。
struct FakeTimeline {
    std::int64_t mono = 1'000'000'000;
    std::int64_t elapsed = 500'000'000;

    void advance(std::int64_t mono_step_ns, std::int64_t elapsed_step_ns) {
        mono += mono_step_ns;
        elapsed += elapsed_step_ns;
    }
};

// ---- ① 稳 --------------------------------------------------------------

void t1_steady() {
    pace::LeadTraceRecorder recorder;
    recorder.set_frame_duration(kFrameNs);
    FakeTimeline clock;

    for (int i = 0; i < 480; ++i) {
        (void)recorder.note(clock.elapsed, clock.mono);
        clock.advance(kFrameNs, kFrameNs);
    }
    const auto trace = recorder.summary();

    CHECK_EQ(trace.samples, 480);
    CHECK_EQ(trace.last_ns, 0);
    CHECK_EQ(trace.max_ns, 0);
    CHECK_EQ(trace.min_ns, 0);
    CHECK_EQ(trace.max_step_ns, 0);
    CHECK_EQ(trace.step_frames, 0);
    CHECK(trace.verdict == pace::LeadVerdict::kSteady);
    CHECK(trace.measured.measured);
    // 理由非空且与 verdict 一致 —— "量到了"那一路的 reason 是空串。
    CHECK(trace.verdict_reason[0] != '\0');
}

// ---- ② 一次阶跃 ---------------------------------------------------------

void t2_stepped() {
    pace::LeadTraceRecorder recorder;
    recorder.set_frame_duration(kFrameNs);
    FakeTimeline clock;

    constexpr std::int64_t kStepNs = 400'000'000;  // 400 ms，与 P14-T 那 4 份同量级
    for (int i = 0; i < 480; ++i) {
        (void)recorder.note(clock.elapsed, clock.mono);
        // 第 200 帧：钟自己往前跳 400 ms，墙一动没动。
        clock.advance(kFrameNs, i == 200 ? kFrameNs + kStepNs : kFrameNs);
    }
    const auto trace = recorder.summary();

    CHECK(trace.verdict == pace::LeadVerdict::kStepped);
    CHECK_EQ(trace.max_step_ns, kStepNs);
    CHECK_EQ(trace.step_frames, 1);
    CHECK_EQ(trace.last_ns, kStepNs);
    // 5.36% 级别的偏移，ppm 必须远大于"晶振频差"的量级（判据 U4 的 2000）。
    CHECK(trace.rate_ppm > 2000.0);
    CHECK(trace.measured.measured);
}

// ---- ③ 匀速跑偏 ---------------------------------------------------------

void t3_ramped() {
    pace::LeadTraceRecorder recorder;
    recorder.set_frame_duration(kFrameNs);
    FakeTimeline clock;

    constexpr std::int64_t kDriftPerFrameNs = 100'000;  // 每帧 0.1 ms ≈ 6000 ppm
    for (int i = 0; i < 480; ++i) {
        (void)recorder.note(clock.elapsed, clock.mono);
        clock.advance(kFrameNs, kFrameNs + kDriftPerFrameNs);
    }
    const auto trace = recorder.summary();

    CHECK(trace.verdict == pace::LeadVerdict::kRamped);
    // 关键：一步一步都没超一帧，所以**不许**有 step。
    CHECK_EQ(trace.step_frames, 0);
    CHECK_EQ(trace.max_step_ns, kDriftPerFrameNs);
    CHECK_EQ(trace.last_ns, 479 * kDriftPerFrameNs);
    // 6000 ppm 上下：分母是 479 帧的墙钟跨度。
    CHECK(trace.rate_ppm > 5000.0 && trace.rate_ppm < 7000.0);
}

// ---- ④ 缺参数不许给结论 ------------------------------------------------

void t4_unknown_without_frame_duration() {
    pace::LeadTraceRecorder recorder;  // 故意不设 frame_duration
    FakeTimeline clock;

    for (int i = 0; i < 100; ++i) {
        (void)recorder.note(clock.elapsed, clock.mono);
        // 造一个"如果真的判，会判成 ramped"的形状：每帧都跑偏一点点。
        clock.advance(kFrameNs, kFrameNs + 500'000);
    }
    const auto trace = recorder.summary();

    CHECK(trace.verdict == pace::LeadVerdict::kUnknown);
    CHECK(!trace.measured.measured);
    CHECK(trace.verdict_reason[0] != '\0');
    // 这一条是本组最要紧的断言：它**不许**落到 kRamped。
    CHECK(trace.verdict != pace::LeadVerdict::kRamped);
    // 数照常记，只是不许下结论 —— 数据在，判决没有。
    CHECK_EQ(trace.samples, 100);
    CHECK(trace.last_ns > 0);
}

// ---- ⑤ 恰好卡在容差线上 ------------------------------------------------

void t5_just_under_one_frame() {
    pace::LeadTraceRecorder recorder;
    recorder.set_frame_duration(kFrameNs);
    FakeTimeline clock;

    constexpr std::int64_t kUnderNs = 15'000'000;  // 0.9 帧
    for (int i = 0; i < 60; ++i) {
        (void)recorder.note(clock.elapsed, clock.mono);
        clock.advance(kFrameNs, i == 30 ? kFrameNs + kUnderNs : kFrameNs);
    }
    const auto trace = recorder.summary();

    // 没过一帧 ⇒ 不算跳。
    CHECK_EQ(trace.step_frames, 0);
    CHECK(trace.verdict != pace::LeadVerdict::kStepped);
    // 末值也没过一帧 ⇒ 不算跑偏。落在 steady。
    CHECK(trace.verdict == pace::LeadVerdict::kSteady);
    CHECK_EQ(trace.max_step_ns, kUnderNs);
}

// ---- 单帧不足 ----------------------------------------------------------

void t6_too_few_samples() {
    pace::LeadTraceRecorder recorder;
    recorder.set_frame_duration(kFrameNs);
    (void)recorder.note(1'000'000'000, 2'000'000'000);

    const auto trace = recorder.summary();
    CHECK_EQ(trace.samples, 1);
    CHECK(trace.verdict == pace::LeadVerdict::kUnknown);
    CHECK(!trace.measured.measured);
    CHECK(trace.verdict_reason[0] != '\0');
}

// ---- 墙偏移：首末与极值 ------------------------------------------------

void t7_wall_offset_extremes() {
    pace::WallOffsetRecorder recorder;
    recorder.note(1'000, 0);
    recorder.note(-5'000'000, 100'000'000);
    recorder.note(2'000, 200'000'000);

    const auto summary = recorder.summary();
    CHECK_EQ(summary.samples, 3);
    CHECK_EQ(summary.first_ns, 1'000);
    CHECK_EQ(summary.last_ns, 2'000);
    CHECK_EQ(summary.worst_ns, -5'000'000);
    CHECK_EQ(summary.worst_media_ns, 100'000'000);
    CHECK_EQ(summary.worst_index, 1);
    CHECK(summary.measured.measured);
}

void t8_wall_offset_empty() {
    pace::WallOffsetRecorder recorder;
    const auto summary = recorder.summary();
    CHECK_EQ(summary.samples, 0);
    CHECK(!summary.measured.measured);
    CHECK(summary.measured.reason[0] != '\0');
}

// ---- 误差上界的两条分支 ------------------------------------------------

void t9_bound_takes_the_larger_term() {
    // 8 s 窗口：一帧（16.67 ms）远大于 50 ppm × 8 s = 0.4 ms。
    CHECK_EQ(pace::wall_reference_bound_ns(kFrameNs, 8'000'000'000), kFrameNs);
    // 600 s 窗口：50 ppm × 600 s = 30 ms，反超一帧。
    CHECK_EQ(pace::wall_reference_bound_ns(kFrameNs, 600'000'000'000), 30'000'000);
    // 跨度 0（没量到）：只剩帧长那一项，不返回 0。
    CHECK_EQ(pace::wall_reference_bound_ns(kFrameNs, 0), kFrameNs);
}

// ---- ⑥ 接线：真接上了，且该沉默时沉默 --------------------------------

pace::DueStep make_step(std::int64_t mono_ns, std::int64_t elapsed_ns, bool with_clock) {
    pace::DueStep step;
    step.frame_duration_ns = kFrameNs;
    step.present_step_ns = 0;
    if (with_clock) {
        step.clock_mono_ns = mono_ns;
        step.clock_elapsed_ns = elapsed_ns;
        step.anchor_pts_ns = 0;
        // `due` 的绝对值只用于"有没有钟读数"这一判断，取非 0 即可。
        step.due_at_ns = mono_ns + elapsed_ns + 1;
    }
    return step;
}

void t10_pace_stats_wiring() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    for (int i = 0; i < 240; ++i) {
        stats.note_presented(125'000, pts, make_step(clock.mono, clock.elapsed, true));
        clock.advance(kFrameNs, kFrameNs);
        pts += kFrameNs;
    }
    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);

    CHECK_EQ(summary.clock_supplied_frames, 240);
    CHECK_EQ(summary.lead_trace.samples, 240);
    CHECK_EQ(summary.lead_trace.last_ns, 0);
    CHECK(summary.lead_trace.verdict == pace::LeadVerdict::kSteady);
    CHECK_EQ(summary.wall_offset.samples, 240);
    // 领先量为 0 时，墙偏移必须**等于** lateness 本身（125 µs）。
    CHECK_EQ(summary.wall_offset.last_ns, 125'000);
    CHECK_EQ(summary.wall_offset.worst_ns, 125'000);
    CHECK(summary.wall_reference_bound_ns > 0);
}

void t11_surface_at_time_feeds_nothing() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    // `surface_at_time`：不带钟读数（`due_at_ns == 0`），但时钟参数是 0。
    for (int i = 0; i < 240; ++i) {
        stats.note_presented(0, pts, make_step(0, 0, false));
        clock.advance(kFrameNs, kFrameNs);
        pts += kFrameNs;
    }
    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);

    CHECK_EQ(summary.clock_supplied_frames, 0);
    // 一个样本都不许喂 —— 喂了 240 个 0 就会得到 verdict=steady 的假观测。
    CHECK_EQ(summary.lead_trace.samples, 0);
    CHECK(summary.lead_trace.verdict == pace::LeadVerdict::kUnknown);
    CHECK(!summary.lead_trace.measured.measured);
    CHECK_EQ(summary.wall_offset.samples, 0);
    CHECK(!summary.wall_offset.measured.measured);
}

void t12_pace_stats_reports_the_step() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    constexpr std::int64_t kStepNs = 400'000'000;
    for (int i = 0; i < 240; ++i) {
        stats.note_presented(125'000, pts, make_step(clock.mono, clock.elapsed, true));
        clock.advance(kFrameNs, i == 120 ? kFrameNs + kStepNs : kFrameNs);
        pts += kFrameNs;
    }
    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);

    CHECK(summary.lead_trace.verdict == pace::LeadVerdict::kStepped);
    CHECK_EQ(summary.lead_trace.max_step_ns, kStepNs);
    // 这是本卡的全部意义：`lateness` 一直是 125 µs（"完美"），
    // 而相对墙的偏移是 −400 ms 量级。
    CHECK_EQ(summary.last_lateness_ns, 125'000);
    CHECK(summary.wall_offset.last_ns < -390'000'000);
    CHECK(summary.wall_offset.last_ns > -410'000'000);
}

}  // namespace

int main() {
    std::printf("P14-U lead trace / wall offset tests\n");
    t1_steady();
    t2_stepped();
    t3_ramped();
    t4_unknown_without_frame_duration();
    t5_just_under_one_frame();
    t6_too_few_samples();
    t7_wall_offset_extremes();
    t8_wall_offset_empty();
    t9_bound_takes_the_larger_term();
    t10_pace_stats_wiring();
    t11_surface_at_time_feeds_nothing();
    t12_pace_stats_reports_the_step();

    std::printf("checked %d, failed %d\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}

// scheduled_span_tests.cpp — P14-AA tests: at_time 档的跨度是谁喂的
//
// 这一卡补的是"没有量程的那一半证据"：`surface_at_time` 档以前从不喂 DueStep，
// 于是 `due_step_supplied = 0`、四跨度全 0、`media_rate` 无从计算 ——
// 而报告里那看起来像"没有位移"。
//
// 守四条：
//   ① scheduled 也喂跨度：`span_frames` 涨，`span_basis = scheduled`；
//   ② presented 的口径不动：`clock_supplied_frames` 仍然只数 presented 那一批；
//   ③ 两条路混着喂时报 `mixed` —— 不假装这一格还是干净的；
//   ④ 没有钟读数（`due_at_ns == 0`）的帧不参与跨度，也不进 `due_step_supplied`。
//
// 纯计算：不依赖时钟、不需要设备。
#include <cstdint>
#include <cstdio>
#include <string>

#include "video/video_plane/core/pace/pace_stats.hpp"

namespace pace = kivo::video::core::pace;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                       \
    do {                                                                  \
        ++g_checked;                                                      \
        if (!(expr)) {                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr); \
        }                                                                 \
    } while (false)

constexpr std::int64_t kFrame = 16'666'667;

pace::DueStep step_at(std::int64_t pts_ns, std::int64_t clock_ns, std::int64_t due_at_ns) {
    return pace::DueStep{.due_step_ns = 0,
                         .frame_duration_ns = kFrame,
                         .present_step_ns = 0,
                         .clock_mono_ns = clock_ns,
                         .clock_elapsed_ns = clock_ns,
                         .anchor_pts_ns = 0,
                         .due_at_ns = due_at_ns};
}

// ---- ① scheduled 也喂跨度 ----------------------------------------------------

void t01_scheduled_feeds_spans() {
    pace::PaceStats stats;
    for (int i = 0; i < 3; ++i) {
        const std::int64_t t = i * kFrame;
        stats.note_scheduled(0);
        stats.note_scheduled_step(step_at(t, t + 1'000'000'000, t + 1'000'000'000), t);
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);
    CHECK(s.presented == 3);
    CHECK(s.span_frames == 3);
    CHECK(s.due_step_supplied == 3);
    CHECK(s.pts_span_ns == 2 * kFrame);
    CHECK(s.elapsed_span_ns == 2 * kFrame);
    CHECK(std::string(s.span_basis) == "scheduled");
    // presented 的口径没被污染。
    CHECK(s.clock_supplied_frames == 0);
}

// ---- ② presented 的口径一个字没变 --------------------------------------------

void t02_presented_unchanged() {
    pace::PaceStats stats;
    for (int i = 0; i < 3; ++i) {
        const std::int64_t t = i * kFrame;
        stats.note_presented(0, t, step_at(t, t + 1'000'000'000, t + 1'000'000'000));
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);
    CHECK(s.presented == 3);
    CHECK(s.clock_supplied_frames == 3);
    CHECK(s.span_frames == 3);
    CHECK(s.pts_span_ns == 2 * kFrame);
    CHECK(std::string(s.span_basis) == "presented");
}

// ---- ③ 两条路混着喂：必须报 mixed --------------------------------------------

void t03_mixed_is_labelled() {
    pace::PaceStats stats;
    for (int i = 0; i < 2; ++i) {
        const std::int64_t t = i * kFrame;
        stats.note_presented(0, t, step_at(t, t + 1'000'000'000, t + 1'000'000'000));
    }
    for (int i = 2; i < 4; ++i) {
        const std::int64_t t = i * kFrame;
        stats.note_scheduled(0);
        stats.note_scheduled_step(step_at(t, t + 1'000'000'000, t + 1'000'000'000), t);
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);
    CHECK(s.span_frames == 4);
    CHECK(s.clock_supplied_frames == 2);
    CHECK(std::string(s.span_basis) == "mixed");
}

// ---- ④ 没有钟读数的帧不参与 --------------------------------------------------

void t04_frames_without_clock_do_not_count() {
    pace::PaceStats stats;
    for (int i = 0; i < 3; ++i) {
        const std::int64_t t = i * kFrame;
        stats.note_scheduled(0);
        // due_at = 0 = 这一帧没有钟读数（surface_at_time 早期档就是这样）
        stats.note_scheduled_step(step_at(t, 0, 0), t);
    }
    const pace::PaceSummary s = stats.summary(kFrame, pace::ClockHealth::kFresh);
    CHECK(s.span_frames == 0);
    // `due_step_supplied` 数的是"参数传没传"，不是"读数有没有" ——
    // 所以这三帧照样算供上了（`due_step_ns` 是 0，但那是位移为 0，不是没供）。
    CHECK(s.due_step_supplied == 3);
    CHECK(s.pts_span_ns == 0);
}

}  // namespace

int main() {
    t01_scheduled_feeds_spans();
    t02_presented_unchanged();
    t03_mixed_is_labelled();
    t04_frames_without_clock_do_not_count();

    if (g_failed == 0) {
        std::printf("P14-AA scheduled span: %d checked, 0 failed\n", g_checked);
        return 0;
    }
    std::printf("P14-AA scheduled span: %d checked, %d failed\n", g_checked, g_failed);
    return 1;
}

// span_segments_tests.cpp — P15-A2 tests: 跨度描述的是哪一段
//
// 这一组守五句话（每条都对应真机上踩到过的一次）：
//   ① 没 seek 的轮次：1 段，四跨度与 `presented` 全部照旧 —— **逐位不变**。
//      起跑时那次 `reset()` 不算段，否则"没 seek"会被读成"2 段"。
//   ② seek 之后：2 段，`pts_span` 只从 seek 后第一帧算起。
//      旧实现里它从整场第一帧算起，于是把"被跳过的 20 秒"当成了播过的时长 ——
//      真机 `media_rate` 因此读出 1.332。
//   ③ seek 之后 `clock_lead_ns` 必须回到段内口径。旧实现跨段相减，
//      一个 20 s 的 seek 会变成 20 s 的"钟领先量"，看起来像钟坏了。
//   ④ 领先量基准必须跟着切：不切的话 seek 那一跳会被判成 `stepped`，
//      而它根本不是钟跳了 —— 那是**我们自己换了时间轴**。
//   ⑤ 生命周期量不许被清：`presented` 记的是整场交出了多少帧，
//      它和"这一段走了多远"是两个作用域。本卡的规矩就是这一条。
//
// 纯桌面计算：假时间轴，不依赖真实时间、不需要设备。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_stats.hpp"

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

/// 假时间轴：墙钟与钟读数各走各的，两个步长可以不同。
struct FakeTimeline {
    std::int64_t mono = 1'000'000'000;
    std::int64_t elapsed = 500'000'000;

    void advance(std::int64_t mono_step_ns, std::int64_t elapsed_step_ns) {
        mono += mono_step_ns;
        elapsed += elapsed_step_ns;
    }
};

pace::DueStep make_step(std::int64_t mono_ns, std::int64_t elapsed_ns) {
    pace::DueStep step;
    step.frame_duration_ns = kFrameNs;
    step.clock_mono_ns = mono_ns;
    step.clock_elapsed_ns = elapsed_ns;
    step.anchor_pts_ns = 0;
    step.due_at_ns = mono_ns + elapsed_ns + 1;  // 非 0 即"有钟读数"
    return step;
}

/// 喂 `count` 帧，返回喂完之后的时间轴状态（方便下一段接上去）。
FakeTimeline feed(pace::PaceStats& stats, FakeTimeline clock, std::int64_t& pts, int count) {
    for (int i = 0; i < count; ++i) {
        stats.note_presented(125'000, pts, make_step(clock.mono, clock.elapsed));
        stats.note_present_mono(clock.mono);
        clock.advance(kFrameNs, kFrameNs);
        pts += kFrameNs;
    }
    return clock;
}

// ---- ① 没 seek：1 段，逐位照旧 ----------------------------------------

void t1_single_segment() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;
    feed(stats, clock, pts, 240);

    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);
    CHECK_EQ(summary.span_segments, 1);
    CHECK_EQ(summary.span_frames, 240);
    CHECK_EQ(summary.presented, 240);
    CHECK_EQ(summary.pts_span_ns, 239 * kFrameNs);
    CHECK_EQ(summary.elapsed_span_ns, 239 * kFrameNs);
    CHECK_EQ(summary.clock_lead_ns, 0);  // 钟与墙 1:1
    CHECK_EQ(summary.lead_trace.samples, 240);
    CHECK(summary.lead_trace.verdict == pace::LeadVerdict::kSteady);
}

// ---- ② 起跑那次 reset 不算段 -------------------------------------------

void t2_boot_reset_is_not_a_segment() {
    pace::PaceStats stats;
    stats.note_anchor_reset();  // `FramePacer::reset()` 在起跑时就会调

    FakeTimeline clock;
    std::int64_t pts = 0;
    feed(stats, clock, pts, 60);

    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);
    // 一帧都没喂过就 reset：那是起跑，不是切段。报 2 段会让"没 seek"变成"seek 过"。
    CHECK_EQ(summary.span_segments, 1);
    CHECK_EQ(summary.pts_span_ns, 59 * kFrameNs);
}

// ---- ③ seek：跨度只算第二段 --------------------------------------------

void t3_seek_scopes_the_span() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    clock = feed(stats, clock, pts, 120);  // 第 1 段

    // seek：锚点重建 + 媒体时间往前跳 20 s。钟读数与墙钟都继续走，
    // 只有 pts 跳 —— 与真机 `seek_to()` 之后的形状一致。
    stats.note_anchor_reset();
    pts += 20'000'000'000;

    clock = feed(stats, clock, pts, 240);  // 第 2 段
    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);

    CHECK_EQ(summary.span_segments, 2);
    // 段内 240 帧 ⇒ 239 个间隔。旧实现这里是 120+240-1 = 359 帧的跨度，
    // 而且因为 pts 跳了 20 s，会把 20 s 的空档算成播过的时长。
    CHECK_EQ(summary.span_frames, 240);
    CHECK_EQ(summary.pts_span_ns, 239 * kFrameNs);
    // 生命周期量：整场 360 帧一帧不少。
    CHECK_EQ(summary.presented, 360);
}

// ---- ④ seek 之后领先量必须回到段内口径 --------------------------------

void t4_clock_lead_after_seek() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    clock = feed(stats, clock, pts, 120);
    stats.note_anchor_reset();
    pts += 20'000'000'000;
    feed(stats, clock, pts, 120);

    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);
    // 钟与墙一直是 1:1 ⇒ 段内领先量恒 0。
    // 跨段相减的旧实现会得到 ±120 帧的差（时钟与墙钟各自从不同起点算），
    // 一个"凭空多出两秒"的数。
    CHECK_EQ(summary.clock_lead_ns, 0);
    CHECK_EQ(summary.lead_trace.last_ns, 0);
    CHECK_EQ(summary.lead_trace.samples, 120);  // 只有第二段
}

// ---- ⑤ 领先量基准跟着切：seek 不算"钟跳了" -----------------------------

void t5_seek_is_not_a_step() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    clock = feed(stats, clock, pts, 120);
    stats.note_anchor_reset();
    // 关键构造：seek 之后钟**自己**往前跳了 20 s（真机上锚点重建后
    // `clock_elapsed_ns` 的量级确实会换），墙钟继续走。
    clock.elapsed += 20'000'000'000;
    pts += 20'000'000'000;
    feed(stats, clock, pts, 240);

    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);
    // 不切基准的话这里会是 stepped、max_step = 20 s —— 一个"钟跳了 20 秒"的
    // 恐怖结论，而事实只是我们自己换了时间轴。
    CHECK(summary.lead_trace.verdict == pace::LeadVerdict::kSteady);
    CHECK_EQ(summary.lead_trace.max_step_ns, 0);
    CHECK_EQ(summary.lead_trace.step_frames, 0);
    // 段内的领先量是 0：`clock_elapsed` 与 `mono` 在第二段里仍然 1:1。
    CHECK_EQ(summary.lead_trace.last_ns, 0);
}

// ---- ⑥ 墙偏移也按段 ----------------------------------------------------

void t6_wall_offset_scoped() {
    pace::PaceStats stats;
    FakeTimeline clock;
    std::int64_t pts = 0;

    clock = feed(stats, clock, pts, 120);
    stats.note_anchor_reset();
    pts += 20'000'000'000;

    // 第二段里视频比墙慢 40 ms（`lateness − lead`，lead 为 0 时就是 lateness）。
    for (int i = 0; i < 120; ++i) {
        stats.note_presented(40'000'000, pts, make_step(clock.mono, clock.elapsed));
        stats.note_present_mono(clock.mono);
        clock.advance(kFrameNs, kFrameNs);
        pts += kFrameNs;
    }
    const auto summary = stats.summary(kFrameNs, pace::ClockHealth::kFresh);
    // 只有第二段的样本；第一段那 120 个 0.125 ms 已被 restart 清掉。
    CHECK_EQ(summary.wall_offset.samples, 120);
    CHECK_EQ(summary.wall_offset.last_ns, 40'000'000);
}

}  // namespace

int main() {
    t1_single_segment();
    t2_boot_reset_is_not_a_segment();
    t3_seek_scopes_the_span();
    t4_clock_lead_after_seek();
    t5_seek_is_not_a_step();
    t6_wall_offset_scoped();

    std::printf("p15a2 span segments: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
